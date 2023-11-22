#pragma once

#include <absl/container/inlined_vector.h>

namespace thorin {

static constexpr size_t Default_Inlined_Size = 4;
template<class T> using Vector               = absl::InlinedVector<T, Default_Inlined_Size>;

template<class R, class S> bool equal(R range1, S range2) {
    if (range1.size() != range2.size()) return false;
    auto j = range2.begin();
    for (auto i = range1.begin(), e = range1.end(); i != e; ++i, ++j)
        if (*i != *j) return false;
    return true;
}

template<class T, class F, size_t N = Default_Inlined_Size> absl::InlinedVector<T, N> vector(size_t size, F f) {
    absl::InlinedVector<T, N> result(size);
    for (size_t i = 0; i != size; ++i) result[i] = f(i);
    return result;
}

template<class T, std::ranges::range R, class F, size_t N = Default_Inlined_Size>
absl::InlinedVector<T, N> vector(const R& range, F f) {
    absl::InlinedVector<T, N> result(std::distance(range.begin(), range.end()));
    auto ri = range.begin();
    for (size_t i = 0; i != result.size(); ++i, ++ri) result[i] = f(*ri);
    return result;
}

template<class T, size_t N, class A, class U>
constexpr typename absl::InlinedVector<T, N, A>::size_type erase(absl::InlinedVector<T, N, A>& c, const U& value) {
    auto it = std::remove(c.begin(), c.end(), value);
    auto r  = c.end() - it;
    c.erase(it, c.end());
    return r;
}

template<class T, size_t N, class A, class Pred>
constexpr typename absl::InlinedVector<T, N, A>::size_type erase_if(absl::InlinedVector<T, N, A>& c, Pred pred) {
    auto it = std::remove_if(c.begin(), c.end(), pred);
    auto r  = c.end() - it;
    c.erase(it, c.end());
    return r;
}

} // namespace thorin
