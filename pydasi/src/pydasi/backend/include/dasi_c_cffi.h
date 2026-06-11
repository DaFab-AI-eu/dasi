typedef int dasi_bool_t;
typedef long dasi_time_t;
struct Dasi;
typedef struct Dasi dasi_t;
struct Key;
typedef struct Key dasi_key_t;
struct Query;
typedef struct Query dasi_query_t;
struct dasi_wipe_t;
typedef struct dasi_wipe_t dasi_wipe_t;
struct dasi_purge_t;
typedef struct dasi_purge_t dasi_purge_t;
struct dasi_list_t;
typedef struct dasi_list_t dasi_list_t;
struct dasi_retrieve_t;
typedef struct dasi_retrieve_t dasi_retrieve_t;
typedef enum dasi_error_values_t
{
  DASI_SUCCESS = 0,
  DASI_ITERATION_COMPLETE = 1,
  DASI_ERROR = 2,
  DASI_ERROR_UNKNOWN = 3,
  DASI_ERROR_BUG = 4,
  DASI_ERROR_USER = 5,
  DASI_ERROR_ITERATOR = 6,
  DASI_ERROR_ASSERT = 7
} dasi_error_enum_t;
const char *dasi_get_error_string(void);
int dasi_version(const char **version);
int dasi_vcs_version(const char **sha1);
int dasi_initialise_api(void);
int dasi_open(dasi_t **dasi, const char *config);
int dasi_close(const dasi_t *dasi);
int dasi_archive(dasi_t *dasi, const dasi_key_t *key, const void *data, long length);
int dasi_flush(dasi_t *dasi);
int dasi_list(dasi_t *dasi, const dasi_query_t *query, dasi_list_t **list);
int dasi_free_list(const dasi_list_t *list);
int dasi_list_count(const dasi_list_t *list, long *count);
int dasi_list_next(dasi_list_t *list);
int dasi_list_attrs(const dasi_list_t *list, dasi_key_t **key, dasi_time_t *timestamp, const char **uri, long *offset, long *length);
int dasi_retrieve(dasi_t *dasi, const dasi_query_t *query, dasi_retrieve_t **retrieve);
int dasi_free_retrieve(const dasi_retrieve_t *retrieve);
int dasi_retrieve_read(dasi_retrieve_t *retrieve, void *data, long *length);
int dasi_retrieve_count(const dasi_retrieve_t *retrieve, long *count);
int dasi_retrieve_next(dasi_retrieve_t *retrieve);
int dasi_retrieve_attrs(const dasi_retrieve_t *retrieve, dasi_key_t **key, dasi_time_t *timestamp, long *offset, long *length);
int dasi_wipe(dasi_t *dasi, const dasi_query_t *query, const dasi_bool_t *doit, const dasi_bool_t *all, dasi_wipe_t **wipe);
int dasi_free_wipe(const dasi_wipe_t *wipe);
int dasi_wipe_next(dasi_wipe_t *wipe);
int dasi_wipe_get_value(const dasi_wipe_t *wipe, const char **value);
int dasi_purge(dasi_t *dasi, const dasi_query_t *query, const dasi_bool_t *doit, dasi_purge_t **purge);
int dasi_free_purge(const dasi_purge_t *purge);
int dasi_purge_next(dasi_purge_t *purge);
int dasi_purge_get_value(const dasi_purge_t *purge, const char **value);
int dasi_new_key(dasi_key_t **key);
int dasi_new_key_from_string(dasi_key_t **key, const char *str);
int dasi_free_key(const dasi_key_t *key);
int dasi_key_set(dasi_key_t *key, const char *keyword, const char *value);
int dasi_key_compare(dasi_key_t *key, dasi_key_t *other, int *result);
int dasi_key_get_index(dasi_key_t *key, int n, const char **keyword, const char **value);
int dasi_key_get(dasi_key_t *key, const char *keyword, const char **value);
int dasi_key_has(dasi_key_t *key, const char *keyword, dasi_bool_t *has);
int dasi_key_count(dasi_key_t *key, long *count);
int dasi_key_erase(dasi_key_t *key, const char *keyword);
int dasi_key_clear(dasi_key_t *key);
int dasi_new_query(dasi_query_t **query);
int dasi_new_query_from_string(dasi_query_t **query, const char *str);
int dasi_free_query(const dasi_query_t *query);
int dasi_query_set(dasi_query_t *query, const char *keyword, const char *values[], int num);
int dasi_query_append(dasi_query_t *query, const char *keyword, const char *value);
int dasi_query_keyword_count(dasi_query_t *query, long *count);
int dasi_query_value_count(dasi_query_t *query, const char *keyword, long *count);
int dasi_query_get(dasi_query_t *query, const char *keyword, int num, const char **value);
int dasi_query_has(dasi_query_t *query, const char *keyword, dasi_bool_t *has);
int dasi_query_erase(dasi_query_t *query, const char *keyword);
int dasi_query_clear(dasi_query_t *query);
