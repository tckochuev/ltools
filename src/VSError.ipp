#ifndef VS_ERROR_IPP
#define VS_ERROR_IPP

#include "VSError.h"

#include <cassert>

#include <boost/function_types/parameter_types.hpp>
#include <boost/mpl/back.hpp>

namespace tc::err
{

template<typename V, typename E>
template<typename... Arg>
Result<V, E>::Result(tag::Value, Arg&&... arg) : m_isError(false)
{
    if constexpr (!std::is_void_v<V>) {
        new (&m_data) Value(std::forward<Arg>(arg)...);
    }
    else {
        static_assert(sizeof...(Arg) == 0 && "void can not be constructed from args");
    }
}

template<typename V, typename E>
template<typename... Arg>
Result<V, E>::Result(tag::Error, Arg&&... arg) : m_isError(true) {
    new (&m_data) Error(std::forward<Arg>(arg)...);
}

template<typename V, typename E>
Result<V, E>::Result(const Result& other) {
    constructFromOther(other);
}

template<typename V, typename E>
Result<V, E>::Result(Result&& other) noexcept {
    constructFromOther(std::move(other));
}

template<typename V, typename E>
template<typename V2, typename E2>
Result<V, E>::Result(const Result<V2, E2>& other) {
    constructFromOther(other);
}

template<typename V, typename E>
template<typename V2, typename E2>
Result<V, E>::Result(Result<V2, E2>&& other) {
    constructFromOther(std::move(other));
}

template<typename V, typename E>
auto Result<V, E>::operator=(const Result& other)-> Result&
{
    if (&other != this) {
        assignFromOther(other);
    }
    return *this;
}

template<typename V, typename E>
auto Result<V, E>::operator=(Result&& other) noexcept -> Result&
{
    if (&other != this) {
        assignFromOther(std::move(other));
    }
    return *this;
}

template<typename V, typename E>
template<typename V2, typename E2>
auto Result<V, E>::operator=(const Result<V2, E2>& other) -> Result&
{
    if (&other != this) {
        assignFromOther(other);
    }
    return *this;
}

template<typename V, typename E>
template<typename V2, typename E2>
auto Result<V, E>::operator=(Result<V2, E2>&& other) -> Result&
{
    if (&other != this) {
        assignFromOther(std::move(other));
    }
    return *this;
}

template<typename V, typename E>
template<typename... Arg>
auto Result<V, E>::emplaceValue(Arg&&... arg) -> Result&
{
    destroy();
    m_isError = false;
    if constexpr (!std::is_void_v<V>) {
        new (&m_data) Value(std::forward<Arg>(arg)...);
    }
    else {
        static_assert(sizeof...(Arg) == 0 && "void can not be constructed from args");
    }
    return *this;
}

template<typename V, typename E>
template<typename... Arg>
auto Result<V, E>::emplaceError(Arg&&... arg) -> Result&
{
    destroy();
    m_isError = true;
    new (&m_data) Error(std::forward<Arg>(arg)...);
    return *this;
}

template<typename V, typename E>
bool Result<V, E>::isError() const {
    return m_isError;
}

template<typename V, typename E>
Result<V, E>::operator bool() const {
    return !isError();
}

template<typename V, typename E>
decltype(auto) Result<V, E>::value() const& {
    return doValue(*this);
}

template<typename V, typename E>
decltype(auto) Result<V, E>::value() const&& {
    return doValue(std::move(*this));
}

template<typename V, typename E>
decltype(auto) Result<V, E>::value()& {
    return doValue(*this);
}

template<typename V, typename E>
decltype(auto) Result<V, E>::value()&& {
    return doValue(std::move(*this));
}

template<typename V, typename E>
decltype(auto) Result<V, E>::operator*() const& {
    return value();
}

template<typename V, typename E>
decltype(auto) Result<V, E>::operator*() const&& {
    return value();
}

template<typename V, typename E>
decltype(auto) Result<V, E>::operator*()& {
    return value();
}

template<typename V, typename E>
decltype(auto) Result<V, E>::operator*()&& {
    return value();
}

template<typename V, typename E>
auto Result<V, E>::operator->() const -> const Value*
{
    return &value();
}

template<typename V, typename E>
auto Result<V, E>::operator->() -> Value*
{
    return &value();
}

template<typename V, typename E>
auto Result<V, E>::error() const& -> const Error& {
    return doError(*this);
}

template<typename V, typename E>
auto Result<V, E>::error() const&& -> const Error&& {
    return doError(std::move(*this));
}

template<typename V, typename E>
auto Result<V, E>::error() & -> Error& {
    return doError(*this);
}

template<typename V, typename E>
auto Result<V, E>::error() && -> Error&& {
    return doError(std::move(*this));
}

template<typename V, typename E>
template<typename F>
auto Result<V, E>::andThen(F&& f) const& {
    return doAndThen(*this, std::forward<F>(f));
}

template<typename V, typename E>
template<typename F>
auto Result<V, E>::andThen(F&& f) const&& {
    return doAndThen(std::move(*this), std::forward<F>(f));
}

template<typename V, typename E>
template<typename F>
auto Result<V, E>::andThen(F&& f)& {
    return doAndThen(*this, std::forward<F>(f));
}

template<typename V, typename E>
template<typename F>
auto Result<V, E>::andThen(F&& f)&& {
    return doAndThen(std::move(*this), std::forward<F>(f));
}

template<typename V, typename E>
template<typename F>
auto Result<V, E>::orElse(F&& f) const& {
    return doOrElse(*this, std::forward<F>(f));
}

template<typename V, typename E>
template<typename F>
auto Result<V, E>::orElse(F&& f) const&& {
    return doOrElse(std::move(*this), std::forward<F>(f));
}

template<typename V, typename E>
template<typename F>
auto Result<V, E>::orElse(F&& f)& {
    return doOrElse(*this, std::forward<F>(f));
}

template<typename V, typename E>
template<typename F>
auto Result<V, E>::orElse(F&& f)&& {
    return doOrElse(std::move(*this), std::forward<F>(f));
}

template<typename V, typename E>
Result<V, E>::~Result() {
    destroy();
}

template<typename V, typename E>
template<typename Res>
decltype(auto) Result<V, E>::doValue(Res&& result)
{
    assert(!result.isError());
    if constexpr (std::is_void_v<V>) {
        return;
    }
    else
    {
        using Val = std::conditional_t<std::is_const_v<std::remove_reference_t<decltype(result)>>, const Value, Value>;
        using Ref = std::conditional_t<std::is_rvalue_reference_v<decltype(result)>, Val&&, Val&>;
        static_assert(!std::is_rvalue_reference_v<Res> || std::is_rvalue_reference_v<Ref>);
        return static_cast<Ref>(*reinterpret_cast<Val*>(result.m_data));
    }
}

template<typename V, typename E>
template<typename Res>
decltype(auto) Result<V, E>::doError(Res&& result)
{
    assert(result.isError());
    using Err = std::conditional_t<std::is_const_v<std::remove_reference_t<decltype(result)>>, const Error, Error>;
    using Ref = std::conditional_t<std::is_rvalue_reference_v<decltype(result)>, Err&&, Err&>;
    static_assert(!std::is_rvalue_reference_v<Res> || std::is_rvalue_reference_v<Ref>);
    return static_cast<Ref>(*reinterpret_cast<Err*>(result.m_data));
}

template<typename V, typename E>
template<typename Res, typename F>
auto Result<V, E>::doAndThen(Res&& result, F&& f)
{
    using R = typename std::conditional_t<
        std::is_void_v<V>,
        std::invoke_result<F>,
        std::invoke_result<F, decltype(std::forward<Res>(result).value())>
    >::type;
    static_assert(isResult<R> && "f should return specialization of Result");
    if (!result.isError())
    {
        if constexpr (std::is_void_v<V>) {
            return std::invoke(std::forward<F>(f));
        }
        else {
            return std::invoke(std::forward<F>(f), std::forward<Res>(result).value());
        }
    }
    else {
        return R(tag::error, std::forward<Res>(result).error());
    }
}

template<typename V, typename E>
template<typename Res, typename F>
auto Result<V, E>::doOrElse(Res&& result, F&& f)
{
    using R = std::invoke_result_t<F, decltype(std::forward<Res>(result).error())>;
    static_assert(isResult<R> && "f should return specialization of Result");
    if (!result.isError())
    {
        if constexpr (std::is_void_v<V>) {
            return R(tag::value);
        }
        else {
            return R(tag::value, std::forward<Res>(result).value());
        }
    }
    else {
        return std::invoke(f, std::forward<Res>(result).error());
    }
}

template<typename V, typename E>
template<typename Res>
auto Result<V, E>::constructFromOther(Res&& other) -> Result&
{
    m_isError = other.isError();
    if (!other.isError())
    {
        if constexpr (!std::is_void_v<V>) {
            new (&m_data) Value(std::forward<Res>(other).value());
        }
    }
    else {
        new (&m_data) Error(std::forward<Res>(other).error());
    }
    return *this;
}

template<typename V, typename E>
template<typename Res>
auto Result<V, E>::assignFromOther(Res&& other) -> Result&
{
    assert(&other != this);
    return destroy().constructFromOther(std::forward<Res>(other));
}

template<typename V, typename E>
auto Result<V, E>::destroy() -> Result&
{
    if (isError()) {
        error().~Error();
    }
    else
    {
        if constexpr (!std::is_void_v<V>) {
            value().~Value();
        }
    }
    return *this;
}

template<typename V, typename E, typename V2, typename E2>
bool operator==(const Result<V, E>& result1, const Result<V2, E2>& result2)
{
    if (!result1.isError()) {
        return !result2.isError() && result1.value() == result2.value();
    }
    else {
        return result2.isError() && result1.error() == result2.error();
    }
}

template<typename Handler, typename Callable, typename... Args>
decltype(auto) invokeAndHandleResult(Handler&& handler, Callable&& callable, Args&&... args)
{
    return std::invoke(std::forward<Handler>(handler), std::invoke(std::forward<Callable>(callable), std::forward<Args>(args)...));
}

template<typename Object, typename Predicate, typename Error>
auto ObjectObtainHandler::operator()(Object&& object, Predicate&& isValid, Error&& error) const
{
    using Result = Result<tc::RemoveCVRef<Object>, tc::RemoveCVRef<Error>>;
    if (std::invoke(std::forward<Predicate>(isValid), object))
    {
        return Result(tc::err::tag::value, std::forward<Object>(object));
    }
    else
    {
        return Result(tc::err::tag::error, std::forward<Error>(error));
    }
}

namespace exc
{

template<typename Object>
auto AllocationHandler::operator()(Object&& object) const
{
    return std::bind(
        ObjectObtainHandler(),
        std::placeholders::_1,
        [](const auto& object){return object != nullptr;},
        std::make_exception_ptr(std::bad_alloc())
    )(std::forward<Object>(object));
}

template<typename T>
T* cast(std::exception_ptr exception)
{
	static_assert(!std::is_reference_v<T>);
	assert(exception);
	try
	{
		std::rethrow_exception(exception);
	}
	catch(T& e)
	{
		return &e;
	}
	catch(...)
	{
		return nullptr;
	}
}

template<typename E, typename F>
bool handle(std::exception_ptr eptr, F&& f)
{
    assert(eptr);
    try
    {
        std::rethrow_exception(eptr);
    }
    catch(E e)
    {
        std::invoke(std::forward<F>(f), std::forward<E>(e));
        return true;
    }
    catch(...)
    {
        return false;
    }
}

template<typename F>
std::enable_if_t<
    boost::function_types::is_callable_builtin<std::decay_t<F>>::value,
    bool
>
handle(std::exception_ptr eptr, F&& f)
{
    using E = typename boost::mpl::back<
        boost::function_types::parameter_types<std::decay_t<F>>
    >::type;
    return handle<E>(std::move(eptr), std::forward<F>(f));
}

template<typename F>
std::enable_if_t<
    boost::function_types::is_callable_builtin<
        decltype(&std::decay_t<F>::operator())
    >::value,
    bool
>
handle(std::exception_ptr eptr, F&& f)
{
    using E = typename boost::mpl::back<
        boost::function_types::parameter_types<
            decltype(&std::decay_t<F>::operator())
        >
    >::type;
    return handle<E>(std::move(eptr), std::forward<F>(f));
}

template<typename E, typename F>
Handler& Handler::handle(F&& f)
{
	if(!handled)
	{
		handled = tc::err::exc::handle<E>(eptr, std::forward<F>(f));
	}
	return *this;
}

template<typename F>
Handler& Handler::handle(F&& f)
{
	if(!handled)
	{
		handled = tc::err::exc::handle(eptr, std::forward<F>(f));
	}
	return *this;
}

} //tc::err::exc

} //tc::err

#endif
