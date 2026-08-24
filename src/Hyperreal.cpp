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
    ErrorPolicy g_policy      = HERR_LOG;
    size_t      g_max_terms   = 0;  // 0 = 不限制

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
void   set_max_terms(size_t n) noexcept { g_max_terms = n; }
size_t max_terms() noexcept            { return g_max_terms; }

// ============================================================
//  Exception 实现
// ============================================================
Exception::Exception(ErrorCode code,
                     const std::string& func,
                     const std::string& msg,
                     const std::string& detail)
    : _code(code), _func(func), _msg(msg), _detail(detail) {}

std::string Exception::code_str() const
{
    std::ostringstream os;
    os << "E" << std::setw(4) << std::setfill('0') << std::uppercase << std::hex << (int)_code << std::dec;
    return os.str();
}

const char* Exception::what() const noexcept
{
    if(_what_cache.empty())
    {
        try {
            _what_cache = format_error(_code, _func, _msg, _detail);
        } catch(...) {
            // noexcept 契约下绝不抛；内存分配失败时退化为静态串
            return "hyper::Exception: oom in what()";
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
            throw Exception(code, f, m, detail);
    }
    return Hyperreal(); // 返回零（空）
}

void Hyperreal::_truncate_to_max()
{
    size_t cap = max_terms();          // ← 通过命名空间自由函数读取
    if(cap == 0) return;
    if(num.size() <= cap) return;
    // num 已按指数降序排列（高指数在前），保留前 cap 项
    num.resize(cap);
}

// ====================构造函数=====================

Hyperreal::Hyperreal(const container& d)
{
    num = d;
    sort_down();
    merge();
    remove0();
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
    // 钩入全局精度上限：归一化后自动截断到 max_terms() 项
    _truncate_to_max();
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
    ans.sort_down();
    ans.remove0();
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
    c.merge();
    c.sort_down();
    c.remove0();
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
    Hyperreal ans(1.0);
    for(int i = 0; i < n; i++)
    {
        ans = ans * (*this);
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
    if(l.num.empty()) return Hyperreal();
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
        temp = temp * (*this) / (i + 1);
        ans += temp;
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
    eps.merge();
    eps.sort_down();
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
    ans.merge();
    ans.sort_down();
    ans.remove0();
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
    eps.merge();
    eps.sort_down();
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
    ans.sort_down();
    ans.merge();
    ans.remove0();
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

int Hyperreal::estimated_precision() const
{
    int min_neg = 0;
    for(const auto& t : num)
    {
        if(t.second < 0 && -t.second > min_neg) min_neg = -t.second;
    }
    return min_neg;
}

double Hyperreal::error_bound() const
{
    // 截断后丢失的低指数项系数绝对值之和
    size_t cap = max_terms();
    if(cap == 0 || num.size() <= cap) return 0.0;
    double sum = 0.0;
    for(size_t i = cap; i < num.size(); ++i)
    {
        sum += std::fabs(num[i].first);
    }
    return sum;
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
    return c;
}

Hyperreal Hyperreal::real_part() const
{
    Hyperreal c;
    for(const auto& t : num)
    {
        if(t.second == 0) c.num.push_back(t);
    }
    return c;
}

Hyperreal Hyperreal::infinite_part() const
{
    Hyperreal c;
    for(const auto& t : num)
    {
        if(t.second > 0) c.num.push_back(t);
    }
    return c;
}

Hyperreal Hyperreal::infinitesimal_part() const
{
    Hyperreal c;
    for(const auto& t : num)
    {
        if(t.second < 0) c.num.push_back(t);
    }
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

}  // namespace hyper
