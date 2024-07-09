#pragma once

#include <boost/range.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/algorithm/cxx11/none_of.hpp>

//These namespaces are provided to minimize changes in code while switching between std/boost library alternatives.
namespace tcstd
{
	namespace ranges
	{
		using namespace boost;
		using namespace boost::range;
		using namespace boost::algorithm;
		namespace views = boost::adaptors;
		namespace comp //compability layer for logically same classes and functions with different names.
		{
			template<typename Iterator>
			using Subrange = iterator_range<Iterator>;

			template<typename... Args>
			auto makeSubrange(Args&&... args) {
				return make_iterator_range(std::forward<Args>(args)...);
			}
		}
	}
};