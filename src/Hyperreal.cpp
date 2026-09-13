#include "hyper/Hyperreal.h"
#include <iomanip>
#include <sstream>
#include <iostream>
#include <cmath>
#include <algorithm>   // std::sort

// ============================================================
//  namespace hyper 实现
//  ------------------------------------------------------------
//  全局状态（错误策略、精度上限）放在匿名命名空间内，
//  仅本翻译单元可见，外部只能通过 hyper::xxx() 自由函数访问。
// ============================================================
namespace hyper {

namespace {
    // 全局状态：内部链接，仅本 .cpp 可见
    ErrorPolicy              g_policy    = HERR_LOG;
    std::optional<size_t>    g_max_terms = std::nullopt;   // nullopt = 不限制
    std::optional<int>       g_exp_min   = std::nullopt;   // nullopt = 不限制下限
    std::optional<int>       g_exp_max   = std::nullopt;   // nullopt = 不限制上限

    // 数学常量（cos / sin 周期归约使用，避免函数内重复定义）
    constexpr double PI     = 3.14159265358979323846;
    constexpr double TWO_PI = 2.0 * PI;
}

// ============================================================
//  全局配置：错误策略
// ============================================================
void set_error_policy(ErrorPolicy p) noexcept { g_policy = p; }
ErrorPolicy error_policy() noexcept          { return g_policy; }

const char* error_message(ErrorCode code) noexcept
{
    switch(code)
    {
        case HRERR_OK:                   return "OK";
        case HRERR_DIVIDE_BY_ZERO:       return "除零错误";
        case HRERR_NEG_INT_POWER:        return "仅支持非负整数幂";
        case HRERR_ZERO_NEG_POWER:       return "0的负次幂无定义";
        case HRERR_EXP_INFINITY:         return "exp(无穷大)无定义";
        case HRERR_LN_ZERO:              return "ln(0)无定义";
        case HRERR_LN_NEGATIVE:          return "ln(非正数)无定义";
        case HRERR_LN_INFINITY:          return "ln(无穷大)无定义";
        case HRERR_SIN_INFINITY:         return "sin(无穷大)无定义";
        case HRERR_COS_INFINITY:         return "cos(无穷大)无定义";
        case HRERR_TAN_INFINITY:         return "tan(无穷大)无定义";
        case HRERR_SINH_INFINITY:        return "sinh(无穷大)无定义";
        case HRERR_COSH_INFINITY:        return "cosh(无穷大)无定义";
        case HRERR_TANH_INFINITY:        return "tanh(无穷大)无定义";
        case HRERR_NOT_INF_INFINITESIMAL:return "仅对x->0有效（最高指数必须<0）";
        default:                         return "未知错误";
    }
}

std::string format_error(ErrorCode code,
                         const std::string& func,
                         const std::string& msg,
                         const std::string& detail)
{
    std::ostringstream os;
    os << "[Hyperreal "
       << "E" << std::setw(4) << std::setfill('0') << std::uppercase << std::hex << code << std::dec
       << "] "
       << func << " : " << msg;
    if(!detail.empty()) os << "  {" << detail << "}";
    return os.str();
}

// ============================================================
//  全局配置：精度上限
// ============================================================
void set_max_terms(size_t n) noexcept           { g_max_terms = n; }
void clear_max_terms() noexcept                 { g_max_terms = std::nullopt; }
std::optional<size_t> max_terms() noexcept      { return g_max_terms; }

void set_exp_range(int min_exp, int max_exp) noexcept
{
    g_exp_min = min_exp;
    g_exp_max = max_exp;
}
void clear_exp_range() noexcept
{
    g_exp_min = std::nullopt;
    g_exp_max = std::nullopt;
}
std::optional<int> exp_range_min() noexcept { return g_exp_min; }
std::optional<int> exp_range_max() noexcept { return g_exp_max; }

// ============================================================
//  hyp_exception 实现
// ============================================================
hyp_exception::hyp_exception(ErrorCode code,
                     const std::string& func,
                     const std::string& msg,
                     const std::string& detail)
    : _code(code), _func(func), _msg(msg), _detail(detail) {}

std::string hyp_exception::code_str() const
{
    std::ostringstream os;
    os << "E" << std::setw(4) << std::setfill('0') << std::uppercase << std::hex << (int)_code << std::dec;
    return os.str();
}

const char* hyp_exception::what() const noexcept
{
    if(_what_cache.empty())
    {
        try {
            _what_cache = format_error(_code, _func, _msg, _detail);
        } catch(...) {
            // noexcept 契约下绝不抛；内存分配失败时退化为静态串
            return "hyper::hyp_exception: oom in what()";
        }
    }
    return _what_cache.c_str();
}

// ============================================================
//  Hyperreal 类内私有：_raise / _truncate_to_max
//  通过命名空间内自由函数读取全局状态
// ============================================================
Hyperreal Hyperreal::_raise(ErrorCode code,
                            const char* func,
                            const char*  msg,
                            const std::string& detail)
{
    // msg 为空时用错误码对应的标准中文描述
    std::string m = msg ? msg : error_message(code);
    std::string f = func ? func : "(anonymous)";

    switch(error_policy())   // ← 通过命名空间自由函数读取
    {
        case HERR_SILENT:
            break;
        case HERR_LOG:
        {
            std::cerr << format_error(code, f, m, detail) << std::endl;
            break;
        }
        case HERR_THROW:
            throw hyp_exception(code, f, m, detail);
    }
    return Hyperreal(); // 返回零（空）
}

void Hyperreal::_truncate_to_max()
{
    if(!max_terms().has_value()) return;     // nullopt = 不限制
    size_t cap = max_terms().value();
    if(num.size() <= cap) return;
    // num 已按指数降序排列（高指数在前），保留前 cap 项
    num.resize(cap);
}

// ====================构造函数=====================

Hyperreal::Hyperreal(const container& d)
{
    num = d;
    normalize();
}

Hyperreal::Hyperreal() = default;

Hyperreal::Hyperreal(double b)
{
    if(b != 0) num.push_back({b, 0});
}

// ====================获取基本信息=====================

void Hyperreal::print() const
{
    if(num.empty()) { std::cout << "0\n"; return; }
    for(size_t i = 0; i < num.size(); i++)
    {
        if(i < num.size() - 1)
        {
            std::cout << num[i].first << "*inf^" << num[i].second << " + ";
        }
        else
        {
            std::cout << num[i].first << "*inf^" << num[i].second;
        }
    }
    std::cout << "\n";
}

void Hyperreal::print(int n) const
{
    if(num.empty()) { std::cout << "0\n"; return; }
    int len = (int)num.size();
    if(n > len) n = len;
    for(int i = 0; i < n; i++)
    {
        if(i < n - 1)
        {
            std::cout << num[i].first << "*inf^" << num[i].second << " + ";
        }
        else
        {
            std::cout << num[i].first << "*inf^" << num[i].second;
        }
    }
    std::cout << "\n";
}

double Hyperreal::get_coe(int exp) const
{
    for(const auto& i : num)
    {
        if(i.second == exp) return i.first;
    }
    return 0;
}

int Hyperreal::get_exp() const
{
    if(num.empty()) return 0;
    return num.front().second;
}

value_type Hyperreal::get_max() const
{
    if(num.empty()) return {0, 0};
    return num.front();
}

const container& Hyperreal::get_num() const
{
    return num;
}

void Hyperreal::print_max() const
{
    if(num.empty()) { std::cout << "0\n"; return; }
    std::cout << num.front().first << "*inf^" << num.front().second << "\n";
}

size_t Hyperreal::size() const
{
    return num.size();
}

// ====================类型判断=====================

bool Hyperreal::is_zero() const
{
    return num.empty();
}

bool Hyperreal::is_real() const
{
    if(num.empty()) return true;
    return num.size() == 1 && num[0].second == 0;
}

bool Hyperreal::is_infinite() const
{
    if(num.empty()) return false;
    return num[0].second > 0;
}

bool Hyperreal::is_infinitesimal() const
{
    if(num.empty()) return false;
    for(const auto& term : num)
        if(term.second >= 0) return false;
    return true;
}

// ====================格式化=====================

void Hyperreal::merge()
{
    for(int i = 0; i < (int)num.size(); i++)
    {
        for(int j = i + 1; j < (int)num.size(); j++)
        {
            if(num[i].second == num[j].second)
            {
                num[i].first += num[j].first;
                num.erase(num.begin() + j);
                j--;
            }
        }
    }
}

// 线性合并版：要求 num 已经 sort_up/sort_down 有序，
// 同指数项必然相邻，单趟扫描 O(n) 完成；语义与 merge() 一致
// （系数累加进先出现的项），对无序输入不做保证——那种情况请用 merge()。
void Hyperreal::merge_sorted()
{
    if(num.size() < 2) return;
    container merged;
    merged.reserve(num.size());  
    merged.push_back(num[0]);
    for(size_t i = 1; i < num.size(); i++)
    {
        if(merged.back().second == num[i].second)
            merged.back().first += num[i].first;
        else
            merged.push_back(num[i]);
    }
    num = std::move(merged);
}

void Hyperreal::sort_up()
{
    std::sort(num.begin(), num.end(), [](const value_type& a, const value_type& b)
    {
        return a.second < b.second;
    });
}

void Hyperreal::sort_down()
{
    std::sort(num.begin(), num.end(), [](const value_type& a, const value_type& b)
    {
        return a.second > b.second;
    });
}

void Hyperreal::remove0()
{
    for(int i = 0; i < (int)num.size(); i++)
    {
        if(num[i].first == 0)
        {
            num.erase(num.begin() + i);
            i--;
        }
    }
}

void Hyperreal::_truncate_by_exp_range()
{
    auto lo = exp_range_min();
    auto hi = exp_range_max();
    if(!lo.has_value() && !hi.has_value()) return;  // 两端均不限制 = 跳过
    // 保留 lo <= exp <= hi 的项（无值的端点视为不限制该端）
    num.erase(
        std::remove_if(num.begin(), num.end(),
                       [lo, hi](const value_type& t) {
                           bool bad = false;
                           if(lo.has_value()) bad = bad || t.second < lo.value();
                           if(hi.has_value()) bad = bad || t.second > hi.value();
                           return bad;
                       }),
        num.end());
}

void Hyperreal::_apply_global_truncation()
{
    _truncate_to_max();
    _truncate_by_exp_range();
}

void Hyperreal::normalize()
{
    sort_down();
    merge_sorted();   // sort_down 后同指数项相邻，用 O(n) 线性合并
    remove0();
    _apply_global_truncation();
}

// ====================运算符重载=====================

Hyperreal Hyperreal::operator+(const Hyperreal& b) const
{
    bool flag;
    Hyperreal ans = b;
    for(const auto& i : num)
    {
        flag = false;
        for(auto& j : ans.num)
        {
            if(i.second == j.second)
            {
                j.first += i.first;
                flag = true;
                break;
            }
        }
        if(!flag) ans.num.push_back(i);
    }
    ans.normalize();
    return ans;
}

Hyperreal Hyperreal::operator+(double b) const
{
    container d = {{b, 0}};
    Hyperreal c(d);
    return c + (*this);
}

Hyperreal operator+(double a, const Hyperreal& b)
{
    return b + a;
}

Hyperreal Hyperreal::operator-() const
{
    Hyperreal ans = (*this);
    for(auto& i : ans.num)
    {
        i.first = -i.first;
    }
    return ans;
}

Hyperreal Hyperreal::operator-(const Hyperreal& b) const
{
    return (*this) + (-b);
}

Hyperreal Hyperreal::operator-(double b) const
{
    container d = {{-b, 0}};
    Hyperreal c(d);
    return c + (*this);
}

Hyperreal operator-(double a, const Hyperreal& b)
{
    return -(b - a);
}

Hyperreal Hyperreal::operator*(const Hyperreal& b) const
{
    container d = {{0, 0}};
    Hyperreal c(d);
    for(const auto& i : num)
    {
        for(const auto& j : b.num)
        {
            c.num.push_back({i.first * j.first, i.second + j.second});
        }
    }
    c.normalize();
    return c;
}

Hyperreal Hyperreal::operator*(double b) const
{
    Hyperreal c;
    if(b == 0) return c;
    for(const auto& i : num)
    {
        c.num.push_back({i.first * b, i.second});
    }
    return c;
}

Hyperreal operator*(double a, const Hyperreal& b)
{
    return b * a;
}

Hyperreal Hyperreal::operator/(const Hyperreal& b) const
{
    return (*this) * b.inv(10);
}

Hyperreal Hyperreal::operator/(double b) const
{
    Hyperreal c;
    if(b == 0)
    {
        return _raise(HRERR_DIVIDE_BY_ZERO, "operator/(double)", nullptr,
                      "尝试除以实数 0.0");
    }
    for(const auto& i : num)
    {
        c.num.push_back({i.first / b, i.second});
    }
    return c;
}

Hyperreal operator/(double a, const Hyperreal& b)
{
    return Hyperreal(a) / b;
}

bool Hyperreal::operator==(const Hyperreal& b) const
{
    size_t len = num.size();
    if(len != b.num.size()) return false;
    for(size_t i = 0; i < len; i++)
    {
        if(num[i].first != b.num[i].first || num[i].second != b.num[i].second)
        {
            return false;
        }
    }
    return true;
}

bool Hyperreal::operator==(double b) const
{
    return *this == Hyperreal(b);
}

bool operator==(double a, const Hyperreal& b)
{
    return b == a;
}

bool Hyperreal::operator!=(const Hyperreal& b) const
{
    return !(*this == b);
}

bool Hyperreal::operator!=(double b) const
{
    return !(*this == Hyperreal(b));
}

bool operator!=(double a, const Hyperreal& b)
{
    return b != a;
}

bool Hyperreal::operator>(const Hyperreal& b) const
{
    Hyperreal diff = *this - b;
    if(diff.num.empty()) return false;
    return diff.num[0].first > 0;
}

bool Hyperreal::operator>(double b) const
{
    return *this > Hyperreal(b);
}

bool operator>(double a, const Hyperreal& b)
{
    return b < a;
}

bool Hyperreal::operator<(const Hyperreal& b) const
{
    Hyperreal diff = *this - b;
    if(diff.num.empty()) return false;
    return diff.num[0].first < 0;
}

bool Hyperreal::operator<(double b) const
{
    return *this < Hyperreal(b);
}

bool operator<(double a, const Hyperreal& b)
{
    return b > a;
}

bool Hyperreal::operator>=(const Hyperreal& b) const
{
    return *this > b || *this == b;
}

bool Hyperreal::operator>=(double b) const
{
    return *this >= Hyperreal(b);
}

bool operator>=(double a, const Hyperreal& b)
{
    return Hyperreal(a) >= b;
}

bool Hyperreal::operator<=(const Hyperreal& b) const
{
    return *this < b || *this == b;
}

bool Hyperreal::operator<=(double b) const
{
    return *this <= Hyperreal(b);
}

bool operator<=(double a, const Hyperreal& b)
{
    return Hyperreal(a) <= b;
}

Hyperreal& Hyperreal::operator+=(const Hyperreal& b)
{
    *this = *this + b;
    return *this;
}

Hyperreal& Hyperreal::operator-=(const Hyperreal& b)
{
    *this = *this - b;
    return *this;
}

Hyperreal& Hyperreal::operator*=(const Hyperreal& b)
{
    *this = *this * b;
    return *this;
}

Hyperreal& Hyperreal::operator/=(const Hyperreal& b)
{
    *this = *this / b;
    return *this;
}

// ====================常用函数=====================

Hyperreal Hyperreal::pow(signed int n) const
{
    if(n == 0) return Hyperreal(1.0);
    if(n < 0)
    {
        return _raise(HRERR_NEG_INT_POWER, "pow(signed int)", nullptr,
                      "接收到 n = " + std::to_string(n));
    }
    // 快速幂：O(log n) 次乘法（低位在前的二进制分解）
    Hyperreal ans(1.0);
    Hyperreal base = *this;
    while(n > 0)
    {
        if(n & 1) ans = ans * base;
        n >>= 1;
        if(n > 0) base = base * base;  // 最后一次平方是多余的，跳过
    }
    return ans;
}

Hyperreal Hyperreal::pow(const Hyperreal& b, int len) const
{
    if(num.empty())
    {
        if(b.num.empty()) return Hyperreal(1.0); // 0^0 = 1
        if(b > 0) return Hyperreal();            // 0^正 = 0
        if(b == 0) return Hyperreal(1.0);        // 0^0 = 1
        return _raise(HRERR_ZERO_NEG_POWER, "pow(Hyperreal)", nullptr);
    }
    Hyperreal l = (*this).ln(len);
    if(l.num.empty()) return Hyperreal(1.0); // ln(x)=0 ⟺ x==1，此时 x^b = 1^b = 1
    Hyperreal bl = b * l;
    return bl.exp(len);
}

Hyperreal Hyperreal::pow(double b, int len) const
{
    return pow(Hyperreal(b), len);
}

Hyperreal Hyperreal::exp(int len) const
{
    if(num.empty()) return Hyperreal(1.0);

    if(get_exp() > 0)
    {
        return _raise(HRERR_EXP_INFINITY, "exp", nullptr);
    }

    double c = 0;
    Hyperreal eps;
    for(const auto& term : num)
    {
        if(term.second == 0) c = term.first;
        else eps.num.push_back(term);
    }

    Hyperreal eps_ans(1.0);
    if(!eps.num.empty())
    {
        Hyperreal temp(1.0);
        for(int i = 1; i <= len; i++)
        {
            temp = temp * eps / i;
            eps_ans += temp;
        }
    }

    return eps_ans * std::exp(c);
}

Hyperreal Hyperreal::ln_1lessx(int len) const
{
    if(num.empty()) return Hyperreal();
    if(get_exp() >= 0)
    {
        return _raise(HRERR_NOT_INF_INFINITESIMAL, "ln_1lessx", nullptr,
                      "get_exp() = " + std::to_string(get_exp()) + "，需 < 0");
    }
    container d = {{-1, 0}};
    Hyperreal ans, temp(d);
    for(int i = 0; i <= len; i++)
    {
        // ln(1-x) = -Σ x^k/k：幂次累乘，系数每次单独除以 k
        // （不能写成 temp * x / (i+1)，否则除法累积成 1/k!）
        temp = temp * (*this);
        ans += temp / (i + 1);
    }
    return ans;
}

Hyperreal Hyperreal::ln_1morex(int len) const
{
    Hyperreal ans = -(*this);
    return ans.ln_1lessx(len);
}

Hyperreal Hyperreal::ln(int len) const
{
    if(num.empty())
    {
        return _raise(HRERR_LN_ZERO, "ln", nullptr, "超实数为 0");
    }
    double c = num[0].first;
    int e = num[0].second;
    if(e > 0)
    {
        return _raise(HRERR_LN_INFINITY, "ln", nullptr,
                      "最高指数 e = " + std::to_string(e));
    }
    if(e < 0)
    {
        return _raise(HRERR_LN_ZERO, "ln", nullptr,
                      "最高指数 e < 0，极限为 0（e = " + std::to_string(e) + "）");
    }
    if(c <= 0)
    {
        return _raise(HRERR_LN_NEGATIVE, "ln", nullptr,
                      "常数项 c = " + std::to_string(c));
    }

    Hyperreal eps;
    for(const auto& term : num)
    {
        eps.num.push_back({term.first / c, term.second});
    }
    eps.normalize();
    bool found = false;
    for(auto& term : eps.num)
    {
        if(term.second == 0)
        {
            term.first -= 1;
            found = true;
            break;
        }
    }
    if(!found)
    {
        eps.num.push_back({-1, 0});
        eps.sort_down();
    }
    eps.remove0();

    Hyperreal ans;
    if(!eps.num.empty())
    {
        ans = eps.ln_1morex(len);
    }

    bool found_zero = false;
    for(auto& term : ans.num)
    {
        if(term.second == 0)
        {
            term.first += std::log(c);
            found_zero = true;
            break;
        }
    }
    if(!found_zero)
    {
        ans.num.push_back({std::log(c), 0});
    }
    ans.normalize();
    return ans;
}

Hyperreal Hyperreal::inv_1lessx(int len) const
{
    if(num.empty()) return Hyperreal(1.0);
    if(get_exp() >= 0)
    {
        return _raise(HRERR_NOT_INF_INFINITESIMAL, "inv_1lessx", nullptr,
                      "get_exp() = " + std::to_string(get_exp()) + "，需 < 0");
    }
    container d = {{1, 0}};
    Hyperreal ans(d), temp(d);
    for(int i = 1; i <= len; i++)
    {
        temp = temp * (*this);
        ans += temp;
    }
    return ans;
}

Hyperreal Hyperreal::inv_1morex(int len) const
{
    Hyperreal ans = -(*this);
    return ans.inv_1lessx(len);
}

Hyperreal Hyperreal::inv(int len) const
{
    if(num.empty())
    {
        return _raise(HRERR_DIVIDE_BY_ZERO, "inv", nullptr, "inv(0) 无定义");
    }
    double c = num[0].first;
    int e = num[0].second;

    Hyperreal eps;
    for(const auto& term : num)
    {
        eps.num.push_back({term.first / c, term.second - e});
    }
    eps.normalize();
    bool found = false;
    for(auto& term : eps.num)
    {
        if(term.second == 0)
        {
            term.first -= 1;
            found = true;
            break;
        }
    }
    if(!found)
    {
        eps.num.push_back({-1, 0});
        eps.sort_down();
    }
    eps.remove0();

    Hyperreal ans;
    if(!eps.num.empty())
    {
        ans = eps.inv_1morex(len);
    }

    if(ans.num.empty())
    {
        ans.num.push_back({1.0 / c, -e});
        ans.sort_down();
        return ans;
    }
    for(auto& t : ans.num)
    {
        t.first = t.first / c;
        t.second = t.second - e;
    }
    ans.normalize();
    return ans;
}

Hyperreal Hyperreal::cos(int len) const
{
    if(num.empty()) return Hyperreal(1.0);
    if(get_exp() > 0)
    {
        return _raise(HRERR_COS_INFINITY, "cos", nullptr);
    }

    double c = 0;
    Hyperreal eps;
    for(const auto& term : num)
    {
        if(term.second == 0) c = term.first;
        else eps.num.push_back(term);
    }

    c = std::fmod(c, TWO_PI);
    if(c >  PI) c -= TWO_PI;
    if(c < -PI) c += TWO_PI;

    Hyperreal sin_eps, cos_eps(1.0);
    if(!eps.num.empty())
    {
        Hyperreal eps2 = eps * eps;
        Hyperreal sin_term = eps, cos_term(1.0);
        sin_eps = sin_term;
        cos_eps = cos_term;
        for(int i = 1; i <= len; i++)
        {
            sin_term = -sin_term * eps2 / ((2 * i) * (2 * i + 1));
            cos_term = -cos_term * eps2 / ((2 * i - 1) * (2 * i));
            sin_eps += sin_term;
            cos_eps += cos_term;
        }
    }

    return cos_eps * std::cos(c) - sin_eps * std::sin(c);
}

Hyperreal Hyperreal::sin(int len) const
{
    if(num.empty()) return Hyperreal();
    if(get_exp() > 0)
    {
        return _raise(HRERR_SIN_INFINITY, "sin", nullptr);
    }

    double c = 0;
    Hyperreal eps;
    for(const auto& term : num)
    {
        if(term.second == 0) c = term.first;
        else eps.num.push_back(term);
    }

    c = std::fmod(c, TWO_PI);
    if(c >  PI) c -= TWO_PI;
    if(c < -PI) c += TWO_PI;

    Hyperreal sin_eps, cos_eps(1.0);
    if(!eps.num.empty())
    {
        Hyperreal eps2 = eps * eps;
        Hyperreal sin_term = eps, cos_term(1.0);
        sin_eps = sin_term;
        cos_eps = cos_term;
        for(int i = 1; i <= len; i++)
        {
            sin_term = -sin_term * eps2 / ((2 * i) * (2 * i + 1));
            cos_term = -cos_term * eps2 / ((2 * i - 1) * (2 * i));
            sin_eps += sin_term;
            cos_eps += cos_term;
        }
    }

    return cos_eps * std::sin(c) + sin_eps * std::cos(c);
}

Hyperreal Hyperreal::tan(int len) const
{
    // tan(x) = sin(x) / cos(x)；cos=0 时由除法 raise
    if(num.empty()) return Hyperreal();
    if(get_exp() > 0)
    {
        return _raise(HRERR_TAN_INFINITY, "tan", nullptr);
    }
    // 除法内部按 inv(10) 展开，级数相乘会让项数远超 len；
    // 这里按 len 截断（与 sin/cos 的 len+1 项约定一致），避免输出膨胀。
    return (sin(len) / cos(len)).truncated((size_t)len + 1);
}

// 双曲函数内部辅助：同时计算 sinh(eps) 与 cosh(eps) 级数
// sinh(eps) = eps + eps^3/3! + eps^5/5! + ...
// cosh(eps) = 1 + eps^2/2! + eps^4/4! + ...
static void sinh_cosh_eps(const Hyperreal& eps, int len,
                          Hyperreal& sh, Hyperreal& ch)
{
    sh = Hyperreal();          // 0（eps 为空时返回）
    ch = Hyperreal(1.0);
    if(eps.is_zero()) return;
    Hyperreal eps2 = eps * eps;
    Hyperreal t_sh = eps, t_ch(1.0);
    sh = t_sh;
    ch = t_ch;
    for(int i = 1; i <= len; i++)
    {
        t_sh = t_sh * eps2 / ((2 * i) * (2 * i + 1));   // 无符号交替
        t_ch = t_ch * eps2 / ((2 * i - 1) * (2 * i));
        sh += t_sh;
        ch += t_ch;
    }
}

Hyperreal Hyperreal::sinh(int len) const
{
    if(num.empty()) return Hyperreal();
    if(get_exp() > 0)
    {
        return _raise(HRERR_SINH_INFINITY, "sinh", nullptr);
    }

    double c = 0;
    Hyperreal eps;
    for(const auto& term : num)
    {
        if(term.second == 0) c = term.first;
        else eps.num.push_back(term);
    }

    // sinh(c+eps) = sinh(c)*cosh(eps) + cosh(c)*sinh(eps)
    Hyperreal sh_eps, ch_eps;
    sinh_cosh_eps(eps, len, sh_eps, ch_eps);
    return ch_eps * std::sinh(c) + sh_eps * std::cosh(c);
}

Hyperreal Hyperreal::cosh(int len) const
{
    if(num.empty()) return Hyperreal(1.0);
    if(get_exp() > 0)
    {
        return _raise(HRERR_COSH_INFINITY, "cosh", nullptr);
    }

    double c = 0;
    Hyperreal eps;
    for(const auto& term : num)
    {
        if(term.second == 0) c = term.first;
        else eps.num.push_back(term);
    }

    // cosh(c+eps) = cosh(c)*cosh(eps) + sinh(c)*sinh(eps)
    Hyperreal sh_eps, ch_eps;
    sinh_cosh_eps(eps, len, sh_eps, ch_eps);
    return ch_eps * std::cosh(c) + sh_eps * std::sinh(c);
}

Hyperreal Hyperreal::tanh(int len) const
{
    // tanh(x) = sinh(x) / cosh(x)
    if(num.empty()) return Hyperreal();
    if(get_exp() > 0)
    {
        return _raise(HRERR_TANH_INFINITY, "tanh", nullptr);
    }
    // 同 tan：除法内部按 inv(10) 展开会膨胀项数，按 len 截断
    return (sinh(len) / cosh(len)).truncated((size_t)len + 1);
}

Hyperreal Hyperreal::abs() const
{
    // 超实数符号由首项（最高指数项）系数决定（高指数项主导）：
    //   |x| =  x   （首项系数 > 0，如 3 - 2*inf^-1 → 保持原样）
    //   |x| = -x   （首项系数 < 0，如 -3 - inf^-1 → 3 + inf^-1）
    // 0 的绝对值为 0。
    if(num.empty()) return Hyperreal();
    if(num[0].first > 0) return *this;
    return -(*this);
}

// ====================数值求值=====================

double Hyperreal::eval(double x) const
{
    double sum = 0;
    for(const auto& term : num)
    {
        sum += term.first * std::pow(x, term.second);
    }
    return sum;
}

// ============================================================
//  精度相关实例方法
// ============================================================

Hyperreal Hyperreal::truncated(size_t n) const
{
    Hyperreal c;
    c.num = num;
    // 降序排序后保留前 n 项
    c.sort_down();
    c.remove0();
    if(n > 0 && c.num.size() > n) c.num.resize(n);
    return c;
}

Hyperreal Hyperreal::truncated_by_exp(int min_exp, int max_exp) const
{
    // 保留指数落在 [min_exp, max_exp] 区间内的项
    Hyperreal c;
    for(const auto& t : num)
    {
        if(t.second >= min_exp && t.second <= max_exp)
            c.num.push_back(t);
    }
    c.sort_down();
    c.remove0();
    return c;
}

// ============================================================
//  分量提取（非标准分析）
// ============================================================

Hyperreal Hyperreal::standard_part() const
{
    Hyperreal c;
    for(const auto& t : num)
    {
        if(t.second >= 0) c.num.push_back(t);
    }
    c.normalize();
    return c;
}

Hyperreal Hyperreal::real_part() const
{
    Hyperreal c;
    for(const auto& t : num)
    {
        if(t.second == 0) c.num.push_back(t);
    }
    c.normalize();
    return c;
}

Hyperreal Hyperreal::infinite_part() const
{
    Hyperreal c;
    for(const auto& t : num)
    {
        if(t.second > 0) c.num.push_back(t);
    }
    c.normalize();
    return c;
}

Hyperreal Hyperreal::infinitesimal_part() const
{
    Hyperreal c;
    for(const auto& t : num)
    {
        if(t.second < 0) c.num.push_back(t);
    }
    c.normalize();
    return c;
}

Hyperreal Hyperreal::principal_term() const
{
    if(num.empty()) return Hyperreal();
    // 找最高指数项
    int max_e = num[0].second;
    double coef = num[0].first;
    for(const auto& t : num)
    {
        if(t.second > max_e)
        {
            max_e = t.second;
            coef = t.first;
        }
    }
    return Hyperreal(container{{coef, max_e}});
}

// ============================================================
//  工厂函数（自由函数）
// ============================================================
Hyperreal inf()
{
    return Hyperreal(container{{1, 1}});
}

Hyperreal eps()
{
    return Hyperreal(container{{1, -1}});
}

Hyperreal make(double c, int e)
{
    if(c == 0) return Hyperreal();
    return Hyperreal(container{{c, e}});
}

// ============================================================
//  微积分算法（自由函数）
//  - f 为 std::function<Hyperreal(const Hyperreal&)>，避免按值传参拷贝
//  - x/x0 按 const Hyperreal& 传
// ============================================================
Hyperreal derivative(const std::function<Hyperreal(const Hyperreal&)>& f,
                     const Hyperreal& x,
                     double eps_coef)
{
    // f'(x) ≈ (f(x + h) - f(x)) / h,  h = eps_coef * inf^-1
    Hyperreal h = make(eps_coef, -1);  // 无穷小
    Hyperreal fx  = f(x);
    Hyperreal fxh = f(x + h);
    return (fxh - fx) / h;
}

Hyperreal limit(const std::function<Hyperreal(const Hyperreal&)>& f,
                const Hyperreal& x0,
                int approach)
{
    // lim_{x→x0} f(x) = st(f(x0 + approach * eps))
    Hyperreal e = eps();
    if(approach < 0) e = -e;
    Hyperreal y = f(x0 + e);
    return y;
}

bool is_continuous(const std::function<Hyperreal(const Hyperreal&)>& f,
                   const Hyperreal& x)
{
    // 非标准分析：f 在 x 连续 ⟺ f(x+eps) - f(x) 是无穷小（或零）
    Hyperreal diff = f(x + eps()) - f(x);
    return diff.is_zero() || diff.is_infinitesimal();
}

}  // namespace hyper
