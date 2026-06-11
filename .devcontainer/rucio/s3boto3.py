# Rucio S3/MinIO protocol implementation using boto3.
# Drop this file into rucio/rse/protocols/ to enable scheme=s3 on an RSE.
#
# RSE protocol registration (one-time setup):
#   rucio rse protocol add <RSE> \
#     --scheme s3 \
#     --hostname <minio-host> \
#     --port 9000 \
#     --prefix /<bucket>/ \
#     --impl rucio.rse.protocols.s3boto3.Default \
#     --domain-json '{"lan":{"read":1,"write":1,"delete":1},
#                     "wan":{"read":1,"write":1,"delete":1,
#                            "third_party_copy_read":1,"third_party_copy_write":1}}'
#
# Required RSE attributes (set with `rucio rse attribute add`):
#   s3_access_key   — S3 access key (or AWS_ACCESS_KEY_ID env var)
#   s3_secret_key   — S3 secret key (or AWS_SECRET_ACCESS_KEY env var)
#   s3_endpoint_url — full endpoint URL, e.g. http://minio:9000

import logging
import os
import tempfile

import boto3
import botocore.exceptions

from rucio.common import exception
from rucio.common.checksum import adler32
from rucio.rse.protocols import protocol


class Default(protocol.RSEProtocol):
    """Rucio RSE protocol for S3-compatible storage (MinIO, AWS S3, etc.)
    using boto3 directly."""

    def __init__(self, protocol_attr, rse_settings, logger=logging.log):
        super().__init__(protocol_attr, rse_settings, logger=logger)

        # Credentials: prefer env vars, then extended_attributes on the protocol
        ext = self.attributes.get('extended_attributes') or {}
        if isinstance(ext, str):
            import json
            try:
                ext = json.loads(ext)
            except Exception:
                ext = {}

        access_key = (
            os.environ.get('AWS_ACCESS_KEY_ID')
            or ext.get('s3_access_key')
            or 'minioadmin'
        )
        secret_key = (
            os.environ.get('AWS_SECRET_ACCESS_KEY')
            or ext.get('s3_secret_key')
            or 'minioadmin'
        )

        hostname = self.attributes['hostname']
        port = self.attributes.get('port', 9000)
        scheme = 'https' if port == 443 else 'http'
        endpoint_url = (
            ext.get('s3_endpoint_url')
            or os.environ.get('MINIO_HOST')
            or f'{scheme}://{hostname}:{port}'
        )
        region_name = os.environ.get('MINIO_REGION') or os.environ.get('AWS_DEFAULT_REGION')

        self._s3 = boto3.client(
            's3',
            endpoint_url=endpoint_url,
            aws_access_key_id=access_key,
            aws_secret_access_key=secret_key,
            region_name=region_name,
            config=boto3.session.Config(signature_version='s3v4'),
        )

        # prefix is  /<bucket>[/optional-root-prefix]/
        prefix = self.attributes.get('prefix', '/').strip('/')
        if not prefix:
            prefix = os.environ.get('MINIO_BUCKET', '').strip('/')
        parts = prefix.split('/', 1)
        self._bucket = parts[0]
        if not self._bucket:
            raise exception.RSEAccessDenied('No S3 bucket configured. Set protocol prefix to /<bucket>/ or define MINIO_BUCKET.')
        self._root = parts[1].rstrip('/') + '/' if len(parts) > 1 and parts[1] else ''

    # ------------------------------------------------------------------ helpers

    def _pfn_to_key(self, pfn: str) -> str:
        """Convert a PFN (s3://host/bucket/path/file) to an S3 object key."""
        parsed = list(self.parse_pfns(pfn).values())[0]
        path = parsed.get('path', '').strip('/')
        name = parsed.get('name', '')
        parts = [p for p in [self._root.rstrip('/'), path, name] if p]
        return '/'.join(parts)

    def _lfns_to_pfns_impl(self, lfns):
        prefix = '/' + self._bucket + '/' + self._root
        pfns = {}
        for lfn in lfns:
            scope, name = str(lfn['scope']), lfn['name']
            if 'path' in lfn and lfn.get('path'):
                rel = lfn['path'].lstrip('/')
            else:
                rel = self._get_path(scope=scope, name=name)
            pfns[f'{scope}:{name}'] = (
                f"s3://{self.attributes['hostname']}:{self.attributes.get('port', 9000)}"
                f"{prefix}{rel}"
            )
        return pfns

    # --------------------------------------------------------- protocol API

    def connect(self):
        """Verify connectivity by checking the bucket exists (creates it if not)."""
        try:
            self._s3.head_bucket(Bucket=self._bucket)
        except botocore.exceptions.ClientError as e:
            code = e.response['Error']['Code']
            if code in ('404', 'NoSuchBucket'):
                try:
                    self._s3.create_bucket(Bucket=self._bucket)
                    self.logger(logging.INFO, f's3boto3: created bucket {self._bucket}')
                except Exception as ce:
                    raise exception.RSEAccessDenied(f'Cannot create bucket {self._bucket}: {ce}')
            else:
                raise exception.RSEAccessDenied(e)

    def close(self):
        pass

    def exists(self, pfn):
        key = self._pfn_to_key(pfn)
        try:
            self._s3.head_object(Bucket=self._bucket, Key=key)
            return True
        except botocore.exceptions.ClientError as e:
            if e.response['Error']['Code'] in ('404', 'NoSuchKey'):
                return False
            raise exception.ServiceUnavailable(e)

    def lfns2pfns(self, lfns):
        lfns = [lfns] if isinstance(lfns, dict) else lfns
        return self._lfns_to_pfns_impl(lfns)

    def pfn2path(self, pfn):
        """Not meaningful for S3 — return the object key."""
        return self._pfn_to_key(pfn)

    def get(self, pfn, dest, transfer_timeout=None):
        key = self._pfn_to_key(pfn)
        try:
            self._s3.download_file(self._bucket, key, dest)
        except botocore.exceptions.ClientError as e:
            if e.response['Error']['Code'] in ('404', 'NoSuchKey'):
                raise exception.SourceNotFound(e)
            raise exception.ServiceUnavailable(e)

    def put(self, source, target, source_dir=None, transfer_timeout=None):
        sf = os.path.join(source_dir, source) if source_dir else source
        if not os.path.exists(sf):
            raise exception.SourceNotFound(f'Source file not found: {sf}')
        key = self._pfn_to_key(target)
        try:
            self._s3.upload_file(sf, self._bucket, key)
        except botocore.exceptions.ClientError as e:
            raise exception.ServiceUnavailable(e)

    def rename(self, pfn, new_pfn):
        old_key = self._pfn_to_key(pfn)
        new_key = self._pfn_to_key(new_pfn)
        try:
            self._s3.copy_object(
                Bucket=self._bucket,
                CopySource={'Bucket': self._bucket, 'Key': old_key},
                Key=new_key,
            )
            self._s3.delete_object(Bucket=self._bucket, Key=old_key)
        except botocore.exceptions.ClientError as e:
            raise exception.ServiceUnavailable(e)

    def delete(self, pfn):
        key = self._pfn_to_key(pfn)
        try:
            self._s3.delete_object(Bucket=self._bucket, Key=key)
        except botocore.exceptions.ClientError as e:
            raise exception.ServiceUnavailable(e)

    def stat(self, pfn):
        key = self._pfn_to_key(pfn)
        try:
            resp = self._s3.head_object(Bucket=self._bucket, Key=key)
            size = resp['ContentLength']
            # Compute adler32 by downloading to a temp file
            with tempfile.NamedTemporaryFile(delete=False) as tmp:
                self._s3.download_file(self._bucket, key, tmp.name)
                checksum = adler32(tmp.name)
                os.unlink(tmp.name)
            return {'filesize': size, 'adler32': checksum}
        except botocore.exceptions.ClientError as e:
            if e.response['Error']['Code'] in ('404', 'NoSuchKey'):
                raise exception.SourceNotFound(e)
            raise exception.ServiceUnavailable(e)
