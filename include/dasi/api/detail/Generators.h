
/// @author Simon Smart
/// @date   Dec 2022

#pragma once

#include <iterator>
#include <memory>


namespace dasi {

//-------------------------------------------------------------------------------------------------

template <typename T>
class APIGeneratorImpl {

public: // methods

    APIGeneratorImpl() = default;
    virtual ~APIGeneratorImpl() = default;

    virtual void next() = 0;

    [[ nodiscard ]]
    virtual const T& value() const = 0;

    [[ nodiscard ]]
    virtual bool done() const = 0;
};

//-------------------------------------------------------------------------------------------------

template <typename T>
class APIIterator : std::iterator<std::input_iterator_tag, const T,
                                  std::ptrdiff_t, const T*, const T> {
public:  // methods
    explicit APIIterator(APIGeneratorImpl<T>& generator) : generator_(generator) {}

    APIIterator& operator++() {
        generator_.next();
        return *this;
    }

    const T& operator*() const { return generator_.value(); }

    const T* operator->() const { return &generator_.value(); }

    [[nodiscard]]
    bool done() const { return generator_.done(); }

private:  // members
    APIGeneratorImpl<T>& generator_;
};

//-------------------------------------------------------------------------------------------------


class APIIteratorSentinel {
    template <typename LHS>
    friend bool operator!=(const LHS& lhs, const APIIteratorSentinel& rhs) {
        (void)rhs;
        return !lhs.done();
    }
    template <typename RHS>
    friend bool operator!=(const APIIteratorSentinel& lhs, const RHS& rhs) {
        (void)lhs;
        return !rhs.done();
    }
    template <typename LHS>
    friend bool operator==(const LHS& lhs, const APIIteratorSentinel& rhs) {
        (void)rhs;
        return lhs.done();
    }
    template <typename RHS>
    friend bool operator==(const APIIteratorSentinel& lhs, const RHS& rhs) {
        (void)lhs;
        return rhs.done();
    }
};

//-------------------------------------------------------------------------------------------------

template <typename T>
class GenericGenerator {

public: // methods

    using const_iterator = APIIterator<T>;

    explicit GenericGenerator(GenericGenerator&& rhs) : impl_{std::move(rhs.impl_)} {}
    explicit GenericGenerator(std::unique_ptr<APIGeneratorImpl<T>>&& impl) : impl_{std::move(impl)} {}
    ~GenericGenerator() = default;

    const_iterator begin() { return APIIterator<T>{*impl_}; }
    static APIIteratorSentinel end() { return {}; }

protected: // members
    std::unique_ptr<APIGeneratorImpl<T>> impl_;
};

//-------------------------------------------------------------------------------------------------

} // namespace dasi
