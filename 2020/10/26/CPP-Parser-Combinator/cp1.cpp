#include <bits/stdc++.h>

template<typename T>
using ParseResult = std::optional<T>;

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
    auto v = p(in);
    if (!v) return {};
    auto vs = seq(ps...)(in);
    if (!vs) return {};
    return std::tuple_cat(std::make_tuple(v.value()), vs.value());
  };
}

int main() {
  auto a = literal("a"), b = literal("b");
  auto ab = seq(a, b);
  std::string_view in{"abc"};
  auto [x, y] = ab(in).value();
  std::cout << x << ' ' << y << std::endl;
}