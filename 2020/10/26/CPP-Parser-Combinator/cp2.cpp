#include <bits/stdc++.h>

template<typename T>
class optional : std::optional<T> {
public:
  optional() : std::optional<T>() {}
  template<typename Q>
  optional(Q&& v) : std::optional<T>(std::forward<Q>(v)) {}

  using std::optional<T>::has_value;
  using std::optional<T>::value;
  using std::optional<T>::value_or;
  using std::optional<T>::operator bool;

  template<typename F, typename Q = std::invoke_result_t<F, T>>
  auto map(F&& f) { // f: T => ?
    if (!has_value()) return optional<Q>();
    return optional<Q>(f(value()));
  }

  template<typename F, typename Q = std::invoke_result_t<F, T>>
  auto flat_map(F&& f) { // f : T => optional<?>
    if (!has_value()) return Q();
    return f(value());
  }

  auto or_else(const optional<T>& alternative) {
    if (!has_value()) return alternative;
    return *this;
  }
};


template<typename T>
using ParseResult = optional<T>;

template<typename T>
using Parser = std::function<ParseResult<T>(std::string_view&)>;



Parser<std::string> literal(const std::string& s) {
  return [=](std::string_view& in)->ParseResult<std::string> {
    if (in.starts_with(s)) {
      in.remove_prefix(s.length());
      return s;
    } else {
      return {};
    }
  };
}

Parser<std::tuple<>> seq() {
  return [](std::string_view&)->ParseResult<std::tuple<>> {
    return std::make_tuple();
  };
}

template<typename T, typename... Ts, typename R = std::tuple<T, Ts...>>
Parser<R> seq(const Parser<T>& p, const Parser<Ts>& ...ps) {
  return [=](std::string_view& in)->ParseResult<R> {
    return p(in).flat_map([&](auto v) {
      return seq(ps...)(in).map([&](auto vs) {
        return std::tuple_cat(std::make_tuple(v), vs);
      });
    });
  };
}

int main() {
  auto a = literal("a"), b = literal("b");
  auto ab = seq(a, b);
  std::string_view in{"abc"};
  auto [x, y] = ab(in).value();
  std::cout << x << ' ' << y << std::endl;
}