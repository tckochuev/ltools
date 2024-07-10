#ifndef VS_DESKTOP_UTILS_H
#define VS_DESKTOP_UTILS_H

#include <iterator>
#include <functional>
#include <type_traits>
#include <unordered_set>

#include <boost/range.hpp>
#include <boost/system/system_error.hpp>
#include <boost/filesystem.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/type_erasure/iterator.hpp>
#include <boost/type_erasure/same_type.hpp>
#include <boost/type_erasure/relaxed.hpp>
#include <boost/type_erasure/typeid_of.hpp>
#include <boost/type_erasure/any.hpp>
#include <boost/range/any_range.hpp>
#include <boost/bimap.hpp>
#include <boost/bimap/unordered_set_of.hpp>

#define DECL_CP_MV_CTORS_BY_DEF(ClassName)\
ClassName(const ClassName&) = default;\
ClassName(ClassName&&) noexcept = default;

#define DECL_DEF_CP_MV_CTORS_BY_DEF(ClassName)\
ClassName() = default;\
DECL_CP_MV_CTORS_BY_DEF(ClassName)

#define DECL_CP_MV_ASSIGN_BY_DEF(ClassName)\
ClassName& operator=(const ClassName&) = default;\
ClassName& operator=(ClassName&&) noexcept = default;

#define DECL_DEF_CP_MV_CTORS_ASSIGN_BY_DEF(ClassName)\
DECL_DEF_CP_MV_CTORS_BY_DEF(ClassName)\
DECL_CP_MV_ASSIGN_BY_DEF(ClassName)

///@brief Ensures that @p ClassName without any state can not be default constructed, copied or moved
///from outside of class definition and has virtual public destructor. Do not expect any specific implementation.
#define INTERFACE(ClassName)\
public:\
virtual ~ClassName() = default;\
protected:\
DECL_DEF_CP_MV_CTORS_ASSIGN_BY_DEF(ClassName)

namespace tc
{

namespace syserr = boost::system;
namespace stdfs = boost::filesystem;

template<typename TArithmetic = int>
struct Resolution
{
	using Arithmetic = TArithmetic;
	Arithmetic width;
	Arithmetic height;
};

template<typename R, typename... Args>
struct NOP
{
	R operator()(Args... args) const {
		return returns;
	}

	R returns;
};

template<typename... Args>
struct NOP<void, Args...>
{
	void operator()(Args... args) const {
		return;
	}
};

//--Meta Utils--//

template<typename Enum>
struct EnumHash
{
	size_t operator()(const Enum& v) const
	{
		using UT = std::underlying_type_t<Enum>;
		return std::hash<UT>()(static_cast<UT>(v));
	}
};

template<typename Container>
using InsertReturnType = decltype(
	std::declval<Container>().insert(
		std::declval<typename Container::const_iterator>(),
		std::declval<const typename Container::value_type&>()
	)
);

namespace tag
{

struct Sequence {};
struct Set {};

}

template<typename T>
using RemoveCVRef = std::remove_const_t<std::remove_reference_t<T>>;

//----//

template<typename LhsType, typename RhsType, typename Comparator>
inline bool equals(const LhsType& lhs, const RhsType& rhs, Comparator comparator)
{
	return !comparator(lhs, rhs) && !comparator(rhs, lhs);
}

class IgnoreIterator
{
public:
	using difference_type = void;
	using value_type = void;
	using pointer = void;
	using reference = void;
	using iterator_category = std::output_iterator_tag;

	template<typename T>
	void operator=(T&&) const {}
	const IgnoreIterator& operator*() const { return *this; }
	IgnoreIterator& operator++() { return *this; }
	IgnoreIterator operator++(int) { return *this; }
};

///@{
/**
 * @brief There are containers(especially user defined) that do not have interface compatible with std::inserter or std::back_inserter.
 * Because of that these containers can not be used in many algorithms that require output iterator to work.
 * This functor can be used together with boost::function_output_iterator to overcome this drawback.
 */
template<typename TContainer>
struct Inserter
{
	using Container = TContainer;

	Inserter(Container& container) : container(container) {}

	template<typename T>
	void operator()(T&& value) const {
		container.get().insert(std::forward<T>(value));
	}

	std::reference_wrapper<Container> container;
};

/**
 * @brief Used as default position provider in PositionalInserter. Returns same insert position after each insertion.
 * Use this when insert does not invalidate position iterator.
 */
struct PersistentInsertPositionProvider
{
public:
	template<typename Container>
	typename Container::const_iterator operator()(Container& container,
												  typename Container::const_iterator position,
												  const InsertReturnType<Container>& insertionResult) const
	{
		return position;
	}

	template<typename Container>
	typename Container::const_iterator operator()(Container& container,
												  typename Container::const_iterator position) const
	{
		return position;
	}
};

/**
 * @brief In contrast to std::inserter this functor provides the ability to customize the way the next insert position is obtained,
 * depending from container itself, previous insert position and insertion result(if not void).
 */
template<typename TContainer, typename TInsertPositionProvider = PersistentInsertPositionProvider>
struct PositionalInserter
{
	using Container = TContainer;
	using InsertPositionProvider = TInsertPositionProvider;

	PositionalInserter(Container& container,
					   typename Container::const_iterator position,
					   InsertPositionProvider insertPositionProvider = InsertPositionProvider{}
	) : container(container), position(position), insertPositionProvider(std::move(insertPositionProvider))
	{}

	template<typename T>
	void operator()(T&& value)
	{
		if constexpr (!std::is_void_v<tc::InsertReturnType<Container>>)
		{
			position = std::invoke(insertPositionProvider,
								   container.get(),
								   position,
								   container.get().insert(position, std::forward<T>(value))
			);
		}
		else
		{
			container.get().insert(position, std::forward<T>(value));
			position = std::invoke(insertPositionProvider,
								   container.get(),
								   position
			);
		}
	}

	std::reference_wrapper<Container> container;
	typename Container::const_iterator position;
	InsertPositionProvider insertPositionProvider;
};
///@}

/**
 * @brief User defined container often internally based on composition with some stl-like container.
 * In such case developer should provide some boilerplate code to make his container satisfy Range or Container concept in order to
 * use it with algorithms that require these, and this class is used to reduce repetition of this boilerplate code.
 */
template<typename TContainer, bool exposeNonConstIterator = false>
class BasedOnContainer
{
protected:
	using Container = TContainer;

public:
	using value_type = typename Container::value_type;
	using reference = typename Container::reference;
	using const_reference = typename Container::const_reference;
	using const_iterator = typename Container::const_iterator;
	using iterator = std::conditional_t<exposeNonConstIterator, typename Container::iterator, const_iterator>;
	using difference_type = typename Container::difference_type;
	using size_type = typename Container::size_type;

	iterator begin() {
		return container().begin();
	}
	const_iterator begin() const {
		return container().begin();
	}
	const_iterator cbegin() const {
		return container().begin();
	}
	iterator end() {
		return container().end();
	};
	const_iterator end() const {
		return container().end();
	}
	const_iterator cend() const {
		return container().end();
	}
	size_type max_size() const {
		return container().max_size();
	}
	size_type size() const {
		return container().size();
	}
	bool empty() const {
		return container().empty();
	}

	virtual ~BasedOnContainer() = default;

protected:
	virtual const Container& container() const = 0;
	Container& container() {
		return const_cast<Container&>(static_cast<const BasedOnContainer*>(this)->container());
	}
};

template<typename TContainer, bool exposeNonConstIterator = false>
class BasedOnReversibleContainer : public BasedOnContainer<TContainer, exposeNonConstIterator>
{
private:
	using Super = BasedOnContainer<TContainer>;

protected:
	using Container = typename Super::Container;
	using Super::container;

public:
	using const_reverse_iterator = typename Container::const_reverse_iterator;
	using reverse_iterator = std::conditional_t<exposeNonConstIterator, typename Container::reverse_iterator, const_reverse_iterator>;

	reverse_iterator rbegin() {
		return container().rbegin();
	}
	const_reverse_iterator rbegin() const {
		return container().rbegin();
	}
	const_reverse_iterator crbegin() const {
		return container().rbegin();
	}
	reverse_iterator rend() {
		return container().rend();
	}
	const_reverse_iterator rend() const {
		return container().rend();
	}
	const_reverse_iterator crend() const {
		return container().rend();
	}
};

/**
 * @brief Helper type for std::visit. From https://en.cppreference.com/w/cpp/utility/variant/visit.
 */
template<typename... Ts>
struct Overloaded : Ts...
{
	using Ts::operator()...;
};

//explicit deduction guide.
template<typename... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

struct Identity
{
	template<typename T>
	T&& operator()(T&& t) const {
		return std::forward<T>(t);
	}
};

///@pre Set's find function supports result of keyExtractor(range_reference<remove_ref<InputRange>>) as input parameter.
/// ExclusionOutputIterator accepts range_iterator<remove_ref<Set>>,
/// IntersectionOutputIterator accepts init list {range_iterator<remove_ref<Set>>, range_iterator<remove_ref<InputRange>>},
/// InclusionOutputIterator accepts range_iterator<remove_ref<InputRange>>;
template<
	typename InputRange, typename Set,
	typename ExclusionOutputIterator,
	typename IntersectionOutputIterator,
	typename InclusionOutputIterator,
	typename KeyExtractor = Identity
>
void compare(InputRange&& range, Set&& set,
			 ExclusionOutputIterator exclusionOutput,
			 IntersectionOutputIterator intersectionOutput,
			 InclusionOutputIterator inclusionOutput,
			 KeyExtractor keyExtractor = KeyExtractor{})
{
	std::unordered_set<typename std::remove_reference_t<Set>::const_pointer> intersected;
	for (auto it = boost::begin(range); it != boost::end(range); ++it)
	{
		if (auto found = set.find(keyExtractor(*it)); found != set.end())
		{
			*intersectionOutput++ = {found, it};
			intersected.insert(&(*found));
		}
		else {
			*inclusionOutput++ = it;
		}
	}
	for (auto it = set.begin(); it != set.end(); ++it)
	{
		if (intersected.find(&(*it)) == intersected.end()) {
			*exclusionOutput++ = it;
		}
	}
}

template<typename T, typename Range>
T fromRange(Range&& range) {
	return T(boost::begin(range), boost::end(range));
}

template<typename InputIt, typename OutputIt, typename Predicate>
InputIt copyWhile(InputIt first, InputIt last, OutputIt output, Predicate predicate)
{
	while (first != last && std::invoke(predicate, *first))
	{
		*output++ = *first++;
	}
	return first;
}

template<typename Container, typename Iterator, typename Predicate>
typename Container::size_type erase_if(
	Container& container,
	Iterator first,
	Iterator last,
	Predicate&& predicate = Predicate()
)
{
	typename Container::size_type oldSize = container.size();
	if constexpr (std::is_move_assignable_v<typename std::iterator_traits<Iterator>::reference>)
	{
		Iterator removedFirst = std::remove_if(first, last, std::forward<Predicate>(predicate));
		container.erase(removedFirst, last);
	}
	else
	{
		//Use non const iterator if this assertion is not satisfied.(This ensures that use of this function is effective as possible.)
		static_assert(!std::is_move_assignable_v<typename std::iterator_traits<typename Container::iterator>::reference>);
		while (first != last)
		{
			if (std::invoke(predicate, *first))
			{
				first = container.erase(first);
			}
			else
			{
				++first;
			}
		}
	}
	return oldSize - container.size();
}

template<typename Container, typename Predicate>
typename Container::size_type erase_if(
	Container& container,
	Predicate&& predicate = Predicate()
)
{
	return erase_if(container, std::begin(container), std::end(container), std::forward<Predicate>(predicate));
}

namespace ter // for type erasure
{

/// @brief Reference uses const Value to workaround Boost.Range bug #10493.
template<typename Value, typename Reference = const Value, typename Difference = std::ptrdiff_t, typename Buffer = boost::use_default>
using AnySinglePassRange = boost::any_range<Value, boost::single_pass_traversal_tag, Reference, Difference, Buffer>;

namespace bter //for boost type erasure
{

template<
	typename Value,
	typename Traversal,
	typename Reference = Value&,
	typename Difference = std::ptrdiff_t
>
using AnyIterator = boost::type_erasure::any<
	boost::mpl::vector<
		boost::type_erasure::iterator<
			Traversal,
			boost::type_erasure::_self,
			Reference,
			Difference
		>,
		boost::type_erasure::same_type<
			typename boost::type_erasure::iterator<
				Traversal,
				boost::type_erasure::_self,
				Reference,
				Difference
			>::value_type,
			std::remove_cv_t<Value>
		>,
		boost::type_erasure::relaxed,
		boost::type_erasure::typeid_<>
	>
>;

} //bte

inline namespace br // for boost range. Faster, but without ability to retrieve erased type.
{

template<
	typename Value,
	typename Traversal,
	typename Reference = Value&,
	typename Difference = std::ptrdiff_t,
	typename Buffer = boost::use_default
>
using AnyIterator = typename boost::range_iterator<
	boost::any_range<
		Value,
		Traversal,
		Reference,
		Difference,
		Buffer
	>
>::type;

} //br

} //type_erasure

template<typename T, typename D>
auto makeUnique(T&& t, D&& d) {
	return std::unique_ptr<std::remove_pointer_t<std::remove_reference_t<T>>, std::decay_t<D>>(
		std::forward<T>(t),
		std::forward<D>(d)
	);
}

template<typename R, typename IndirectPointer>
auto makeDirectDeleter(R(*deleter)(IndirectPointer)) {
	return [deleter](std::remove_pointer_t<IndirectPointer> data) {
		deleter(&data);
	};
}

inline double twipsToPixels(double twips, double dpi) {
	return twips / 1440.0 * dpi;
}

inline double pointsToPixels(double points, double dpi) {
	return (points / 72.0) * dpi;
}

template<typename X, typename Y>
using UnorderedBimap = boost::bimap<
	boost::bimaps::unordered_set_of<X>,
	boost::bimaps::unordered_set_of<Y>
>;

inline std::unique_ptr<char[]> strdup(const char* s)
{
	assert(s);
	auto size = std::strlen(s);
	auto dupped = std::make_unique<char[]>(size + 1);
	std::copy(s, s + size, dupped.get());
	dupped[size] = 0;
	return dupped;
}

} //namespace tc

#endif