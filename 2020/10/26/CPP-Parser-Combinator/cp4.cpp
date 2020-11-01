#include <bits/stdc++.h>

template<typename T>
class TD;

template<typename T, typename... List>
constexpr bool is_in_list = (std::is_same_v<T, List> || ...);

template<typename...>
struct variant_concat;

template<typename T, typename... Ts>
struct variant_concat<T, std::variant<Ts...>> {
  using type = std::variant<T, Ts...>;
};

template<typename T, typename... Ts>
struct unique_variant {
  using type = std::conditional_t<
      is_in_list<T, Ts...>,
      typename unique_variant<Ts...>::type,
      typename variant_concat<T, typename unique_variant<Ts...>::type>::type
  >;
};

template<typename T>
struct unique_variant<T> {
  using type = std::variant<T>;
};

template<typename... Ts>
using unique_variant_t = typename unique_variant<Ts...>::type;

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



template<typename F, typename T, typename R = std::invoke_result_t<F, T>>
Parser<R> operator % (const Parser<T>& p, F&& f) {
  return [=](std::string_view& in)->ParseResult<R> {
    return p(in).map([&](T v) { return f(v); });
  };
}

template<typename F, typename... Ts, typename R = std::invoke_result_t<F, Ts...>>
Parser<R> operator % (const Parser<std::tuple<Ts...>>& p, F&& f) {
  return [=](std::string_view& in)->ParseResult<R> {
    return p(in).map([&](auto v) { return std::apply(f, v); });
  };
}

#define RESOLVE_OVERLOAD(...) \
	[](auto&&...args)->decltype(auto){return __VA_ARGS__(std::forward<decltype(args)>(args)...);}

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

template<typename T>
Parser<T> attempt(const Parser<T>& p) {
  return [=](std::string_view& in)->ParseResult<T> {
    auto in_bak = in;
    auto v = p(in);
    if (!v) in = in_bak;
    return v;
  };
}

namespace detail {
  template<typename R>
  Parser<R> alt() {
    return [](std::string_view&)->ParseResult<R> {
      return {};
    };
  }

  template<typename R, typename T, typename... Ts>
  Parser<R> alt(const Parser<T>& p, const Parser<Ts>& ...ps) {
    return [=](std::string_view& in)->ParseResult<R> {
      return attempt(p)(in).map([](auto v){ return R{v}; })
          .or_else(alt<R>(ps...)(in));
    };
  }

  template<typename... Ts>
  auto flat(const std::variant<Ts...>& v) {
    if constexpr (sizeof...(Ts) == 1) {
      return std::get<0>(v);
    } else {
      return v;
    }
  }
}

template<typename... Ts>
auto alt(const Parser<Ts>& ...ps) {
  return detail::alt<unique_variant_t<Ts...>>(ps...) % RESOLVE_OVERLOAD(detail::flat);
}

template<typename T, typename R = std::vector<T>>
Parser<R> many(const Parser<T>& p) {
  return [=](std::string_view& in)->ParseResult<R> {
    R r;
    ParseResult<T> v;
    while ((v = attempt(p)(in))) {
      r.emplace_back(v.value());
    }
    return r;
  };
}

template<typename R = char>
Parser<R> ch(const std::function<bool(char)>& f) {
  return [=](std::string_view& in)->ParseResult<R> {
    if (in.empty() || !f(in[0])) return {};
    char c = in[0];
    in.remove_prefix(1);
    return c;
  };
}

template<typename T>
Parser<T> lazy(const Parser<T>& p) {
  return [p_addr = &p](std::string_view& in)->ParseResult<T> {
    return (*p_addr)(in);
  };
}

int main() {
  auto eval = [](int a, std::string op, int b) {
    if (op == "*") return a * b;
    if (op == "/") return a / b;
    if (op == "+") return a + b;
    if (op == "-") return a - b;
    assert(0);
  };

  Parser<int> exp;
  Parser<int> number = many(ch(isdigit)) % [](std::vector<char> v) {
    int r = 0;
    for (char c: v) r = r * 10 + c - '0';
    return r;
  };

  Parser<int> factor = alt(
      seq(literal("("), lazy(exp), literal(")")) % [](auto x){ return std::get<1>(x); },
      number
  );

  Parser<int> term = alt(
      seq(factor, literal("*"), factor) % eval,
      seq(factor, literal("/"), factor) % eval,
      factor
  );
  exp = alt(
      seq(term, literal("+"), term) % eval,
      seq(term, literal("-"), term) % eval,
      term
  );

  std::string_view in = "4+(1+2)*3";
  std::cout << exp(in).value() << std::endl;
}