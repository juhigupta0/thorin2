#include "thorin/fe/lexer.h"

#include "thorin/world.h"

namespace thorin {

bool issign(int i) { return i == '+' || i == '-'; }

Lexer::Lexer(World& world, std::string_view filename, std::istream& stream)
    : world_(world)
    , loc_{filename, {1, 1}, {1, 1}}
    , peek_({0, Pos(1, 0)})
    , stream_(stream)
{
    next();            // fill peek
    accept(utf8::BOM); // eat utf-8 BOM if present

#define CODE(t, str) keywords_[str] = Tok::Tag::t;
    THORIN_KEY(CODE)
#undef CODE
}

char32_t Lexer::next() {
    for (bool ok = true; true; ok = true) {
        char32_t result = peek_.c32;
        peek_.c32       = stream_.get();
        loc_.finis      = peek_.pos;

        if (eof()) return result;

        switch (auto n = utf8::num_bytes(peek_.c32)) {
            case 0: ok = false; break;
            case 1: /*do nothing*/ break;
            default:
                peek_.c32 = utf8::first(peek_.c32, n);

                for (size_t i = 1; ok && i != n; ++i) {
                    if (auto x = utf8::is_valid(stream_.get()))
                        peek_.c32 = utf8::append(peek_.c32, *x);
                    else
                        ok = false;
                }
        }

        if (peek_.c32 == '\n') {
            ++peek_.pos.row;
            peek_.pos.col = 0;
        } else {
            ++peek_.pos.col;
        }

        if (ok) return result;

        errln("{}, invalid UTF-8 character", peek_.pos);
    }
}

Tok Lexer::lex() {
    while (true) {
        loc_.begin = peek_.pos;
        str_.clear();

        if (eof()) return tok(Tok::Tag::M_eof);
        if (accept_if(isspace)) continue;

        // clang-format off
        // delimiters
        if (accept( '(')) return tok(Tok::Tag::D_paren_l);
        if (accept( ')')) return tok(Tok::Tag::D_paren_r);
        if (accept( '[')) return tok(Tok::Tag::D_bracket_l);
        if (accept( ']')) return tok(Tok::Tag::D_bracket_r);
        if (accept( '{')) return tok(Tok::Tag::D_brace_l);
        if (accept( '}')) return tok(Tok::Tag::D_brace_r);
        if (accept(U'«')) return tok(Tok::Tag::D_quote_l);
        if (accept(U'»')) return tok(Tok::Tag::D_quote_r);
        if (accept(U'‹')) return tok(Tok::Tag::D_angle_l);
        if (accept(U'›')) return tok(Tok::Tag::D_angle_r);

        // Punctuators
        if (accept( '=')) return tok(Tok::Tag::P_assign);
        if (accept( ',')) return tok(Tok::Tag::P_comma);
        if (accept( '.')) return tok(Tok::Tag::P_dot);
        if (accept(U'∷')) return tok(Tok::Tag::P_colon_colon);
        if (accept( ':')) {
            if (accept(':')) return tok(Tok::Tag::P_colon_colon);
            return tok(Tok::Tag::P_colon);
        }

        // binder
        if (accept(U'λ')) return tok(Tok::Tag::B_lam);
        if (accept(U'∀')) return tok(Tok::Tag::B_forall);
        if (accept('\\')) {
            if (accept('/')) return tok(Tok::Tag::B_forall);
            return tok(Tok::Tag::B_lam);
        }
        // clang-format on

        if (accept('/')) {
            if (accept('*')) {
                eat_comments();
                continue;
            }
            if (accept('/')) {
                while (!eof() && peek_.c32 != '\n') next();
                continue;
            }

            errln("{}:{}: invalid input char '/'; maybe you wanted to start a comment?", loc_.file, peek_.pos);
            continue;
        }

        // identifier or keyword
        if (accept_if([](int i) { return i == '_' || isalpha(i); })) {
            while (accept_if([](int i) { return i == '_' || isalpha(i) || isdigit(i); })) {}
            if (auto i = keywords_.find(str_); i != keywords_.end()) return tok(i->second); // keyword
            return {loc(), world_.sym(str_, world_.dbg(loc()))};                            // identifier
        }

        if (isdigit(peek_.c32) || issign(peek_.c32)) return lex_lit();

        errln("{}:{}: invalid input char '{}'", loc_.file, peek_.pos, (char)peek_.c32);
        next();
    }
}

Tok Lexer::lex_lit() {
    int base = 10;

    // clang-format off
    auto parse_digits = [&]() {
        switch (base) {
            case  2: while (accept_if([](int i) { return '0' <= i && i <= '1'; })) {} break;
            case  8: while (accept_if([](int i) { return '0' <= i && i <= '7'; })) {} break;
            case 10: while (accept_if(isdigit)) {} break;
            case 16: while (accept_if(isxdigit)) {} break;
        }
    };

    bool sign = accept_if(issign);

    // prefix starting with '0'
    if (accept('0', false)) {
        if      (accept('b', false)) base = 2;
        else if (accept('x', false)) base = 16;
        else if (accept('o', false)) base = 8;
    }

    parse_digits();

    bool is_float = false;
    if (base == 10) {
        // parse fractional part
        if (accept('.')) {
            is_float = true;
            parse_digits();
        }

        // parse exponent
        if (accept_if([](int i) { return i == 'e' || i == 'E'; })) {
            is_float = true;
            if (accept_if(issign)) {}
            parse_digits();
        }
    }

    if (is_float) return {loc_, r64(strtod  (str_.c_str(), nullptr      ))};
    if (sign)     return {loc_, u64(strtoll (str_.c_str(), nullptr, base))};
    else          return {loc_, u64(strtoull(str_.c_str(), nullptr, base))};
    // clang-format on
}

void Lexer::eat_comments() {
    while (true) {
        while (!eof() && peek_.c32 != '*') next();
        if (eof()) {
            errln("{}:{}: non-terminated multiline comment", loc_);
            return;
        }
        next();
        if (accept('/')) break;
    }
}

} // namespace thorin