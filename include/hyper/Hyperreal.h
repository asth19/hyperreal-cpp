#ifndef HYPER_HYPERREAL_H
#define HYPER_HYPERREAL_H

// ---- 头文件瘦身：只包含实际用到的标准头，不引入 bits/stdc++.h ----
#include <vector>
#include <string>
#include <exception>
#include <functional>
#include <utility>   // std::pair
#include <cstddef>   // size_t

// ============================================================
//  namespace hyper
//  ------------------------------------------------------------
//  本命名空间封装超实数库的全部公开符号：
//    - 配置类自由函数：错误策略、精度上限
//    - 异常类：hyp_exception
//    - 主类：Hyperreal
//    - 工厂函数：inf() / eps() / make(c,e)
//    - 算法函数：derivative() / limit()
//  类内仅保留与"具体对象"绑定的成员，全局状态由命名空间内
//  自由函数管理（更接近 STL 风格）。
//  命名规范：公开 API 统一采用 snake_case。
// ============================================================
namespace hyper {

// 公共类型别名（避免在类外反复写 std::pair<double,int>）
using value_type = std::pair<double, int>;
using container  = std::vector<value_type>;

// ============================================================
//  标准化错误处理：错误码枚举
//  范围 HRERR_xxx，0x01xx 算术类 / 0x02xx 数学函数类 / 0x03xx 参数类
// ============================================================
enum ErrorCode {
    HRERR_OK                  = 0x0000,

    // 算术类：除零 / 非法幂次
    HRERR_DIVIDE_BY_ZERO      = 0x0101,  // 除零错误（/ 0 或 inv(0)）
    HRERR_NEG_INT_POWER       = 0x0102,  // 仅支持非负整数幂（pow(signed int) n<0）
    HRERR_ZERO_NEG_POWER      = 0x0103,  // 0 的负次幂无定义

    // 数学函数类：自变量无定义
    HRERR_EXP_INFINITY        = 0x0201,  // exp(无穷大) 无定义
    HRERR_LN_ZERO             = 0x0211,  // ln(0) 无定义
    HRERR_LN_NEGATIVE         = 0x0212,  // ln(非正数) 无定义
    HRERR_LN_INFINITY         = 0x0213,  // ln(无穷大) 无定义
    HRERR_SIN_INFINITY        = 0x0221,  // sin(无穷大) 无定义
    HRERR_COS_INFINITY        = 0x0222,  // cos(无穷大) 无定义

    // 参数有效性：内部辅助函数收敛域
    HRERR_NOT_INF_INFINITESIMAL = 0x0301 // ln_1lessx / inv_1lessx 仅对 x->0 有效
};

// 错误处理策略
enum ErrorPolicy {
    HERR_SILENT   = 0,   // 静默：只返回零 Hyperreal，不打印不抛
    HERR_LOG      = 1,   // 日志：向 cerr 打印结构化错误，返回零 Hyperreal（默认）
    HERR_THROW    = 2    // 抛异常：抛出 hyp_exception，调用方 try/catch
};

// ============================================================
//  hyp_exception：继承 std::exception，携带错误码与消息
// ============================================================
class hyp_exception : public std::exception
{
public:
    hyp_exception(ErrorCode code,
              const std::string& func,
              const std::string& msg,
              const std::string& detail = "");

    // 返回形如 "[Hyperreal E0101] operator/(double) : 除零错误" 的完整信息
    const char* what()  const noexcept override;

    ErrorCode code() const noexcept { return _code; }
    const std::string& func() const noexcept { return _func; }
    const std::string& msg()  const noexcept { return _msg;  }
    const std::string& detail() const noexcept { return _detail; }

    // 转十六进制错误码字符串，如 "E0101"
    std::string code_str() const;

private:
    ErrorCode          _code;
    std::string        _func;
    std::string        _msg;
    std::string        _detail;
    mutable std::string _what_cache;
};

// ============================================================
//  全局配置（自由函数）
//  - 错误策略
//  - 精度上限
//  - 错误码翻译/格式化
// ============================================================

// 设置/查询错误处理策略（全局生效）
void        set_error_policy(ErrorPolicy p) noexcept;
ErrorPolicy error_policy() noexcept;

// 把错误码翻译成默认中文描述
const char* error_message(ErrorCode code) noexcept;

// 结构化错误日志格式（即使不抛异常，也按同样格式打印，方便检索）
std::string format_error(ErrorCode code,
                         const std::string& func,
                         const std::string& msg,
                         const std::string& detail = "");

// 设置/查询最大保留项数（防止 pow/级数 等操作无限膨胀向量），0 表示不限制
void   set_max_terms(size_t n) noexcept;
size_t max_terms() noexcept;

// ============================================================
//  Hyperreal 主体
//  ------------------------------------------------------------
//  类内仅保留与"具体对象"绑定的成员；
//  全局状态通过上面的自由函数访问。
// ============================================================
class Hyperreal
{
private:
    container num; // first为inf项系数，second为inf项指数

    // 统一报错入口：按策略处理 + 返回零 Hyperreal（方便链式 return）
    // 通过 hyper::error_policy() 读取全局策略
    [[nodiscard]] static Hyperreal _raise(ErrorCode code,
                                          const char* func,
                                          const char* msg,
                                          const std::string& detail = "");

    // 内部：按 hyper::max_terms() 截断到最高 n 项（保留高指数项）
    void _truncate_to_max();

public:
// ====================构造函数=====================
    explicit Hyperreal(const container& d);   // const& 传参，避免不必要的 vector 拷贝
    Hyperreal();                              // 构造函数，初始化超实数为0
    explicit Hyperreal(double b);             // 构造函数，初始化超实数为实数b（explicit 防隐式转换）

// ====================获取基本信息=====================
    void print() const;                       // 打印超实数
    void print(int n) const;                  // 打印超实数，前n位
    double get_coe(int exp) const;            // 获取某指数项系数
    int get_exp() const;                      // 获取最大指数
    value_type get_max() const;               // 获取最大指数项
    const container& get_num() const;         // 获取超实数系数指数项向量
    void print_max() const;                   // 打印最大指数项
    size_t size() const;                      // 获取超实数系数指数项向量大小

// ====================精度相关（实例方法）=====================
    // 对当前对象应用指定上限（保留高指数项），返回新对象
    Hyperreal truncated(size_t n) const;
    // 估算当前精度：最高负指数的绝对值（无负指数时返回 0）
    int estimated_precision() const;
    // 误差上界：被截断项的绝对值之和
    double error_bound() const;

// ====================类型判断=====================
    bool is_zero() const;          // 判断是否为0
    bool is_real() const;          // 判断是否为实数
    bool is_infinite() const;      // 判断是否为无穷大
    bool is_infinitesimal() const; // 判断是否为无穷小

// ====================格式化=====================
    void merge();       // 合并相同指数项
    void sort_up();     // 按指数升序排序
    void sort_down();   // 按指数降序排序
    void remove0();     // 移除系数为0的指数项

// ====================运算符重载=====================
    Hyperreal operator+(const Hyperreal& b) const;  // 加法
    Hyperreal operator+(double b) const;            // 加法，右操作数为实数
    friend Hyperreal operator+(double a, const Hyperreal& b);

    Hyperreal operator-() const;                    // 负号
    Hyperreal operator-(const Hyperreal& b) const;  // 减法
    Hyperreal operator-(double b) const;            // 减法，右操作数为实数
    friend Hyperreal operator-(double a, const Hyperreal& b);

    Hyperreal operator*(const Hyperreal& b) const;  // 乘法
    Hyperreal operator*(double b) const;            // 乘法，右操作数为实数
    friend Hyperreal operator*(double a, const Hyperreal& b);

    Hyperreal operator/(const Hyperreal& b) const;  // 除法
    Hyperreal operator/(double b) const;            // 除法，右操作数为实数
    friend Hyperreal operator/(double a, const Hyperreal& b);

    bool operator==(const Hyperreal& b) const;      // 等于
    bool operator==(double b) const;                // 等于，右操作数为实数
    friend bool operator==(double a, const Hyperreal& b);

    bool operator!=(const Hyperreal& b) const;      // 不等于
    bool operator!=(double b) const;                // 不等于，右操作数为实数
    friend bool operator!=(double a, const Hyperreal& b);

    bool operator>(const Hyperreal& b) const;       // 大于
    bool operator>(double b) const;                 // 大于，右操作数为实数
    friend bool operator>(double a, const Hyperreal& b);

    bool operator<(const Hyperreal& b) const;       // 小于
    bool operator<(double b) const;                 // 小于，右操作数为实数
    friend bool operator<(double a, const Hyperreal& b);

    bool operator>=(const Hyperreal& b) const;      // 大于等于
    bool operator>=(double b) const;                // 大于等于，右操作数为实数
    friend bool operator>=(double a, const Hyperreal& b);

    bool operator<=(const Hyperreal& b) const;      // 小于等于
    bool operator<=(double b) const;                // 小于等于，右操作数为实数
    friend bool operator<=(double a, const Hyperreal& b);

    Hyperreal& operator+=(const Hyperreal& b);  // 加法赋值（返回 *this，支持链式）
    Hyperreal& operator-=(const Hyperreal& b);  // 减法赋值
    Hyperreal& operator*=(const Hyperreal& b);  // 乘法赋值
    Hyperreal& operator/=(const Hyperreal& b);  // 除法赋值

// ====================常用函数=====================
    Hyperreal pow(signed int n) const;                              // 指数
    Hyperreal pow(const Hyperreal& b, int len = 10) const;           // 指数，右操作数为超实数
    Hyperreal pow(double b, int len = 10) const;                      // 指数，右操作数为实数

    Hyperreal exp(int len) const;                                   // 指数函数
    Hyperreal ln_1lessx(int len) const;                             // 自然对数函数，x<1
    Hyperreal ln_1morex(int len) const;                             // 自然对数函数，x>=1
    Hyperreal ln(int len) const;                                    // 自然对数函数
    Hyperreal inv_1lessx(int len) const;                            // 1/(1-x)函数，x->0
    Hyperreal inv_1morex(int len) const;                            // 1/(1+x)函数，x->0
    Hyperreal inv(int len) const;                                   // 1/x函数
    Hyperreal cos(int len) const;                                   // 余弦函数
    Hyperreal sin(int len) const;                                   // 正弦函数

// ====================数值求值=====================
    double eval(double x) const;                                    // 求值，x为实数

// ====================分量提取（非标准分析）=====================
    // 标准部分：丢弃所有无穷小项，仅保留指数 >= 0 的项
    Hyperreal standard_part() const;
    // 实数部分：仅保留指数 = 0 的常数项
    Hyperreal real_part() const;
    // 无穷大部分：仅保留指数 > 0 的项
    Hyperreal infinite_part() const;
    // 无穷小部分：仅保留指数 < 0 的项
    Hyperreal infinitesimal_part() const;
    // 最高次项（首项）
    Hyperreal principal_term() const;
};

// ============================================================
//  工厂函数（Hyperreal 定义后才能返回完整对象）
// ============================================================
Hyperreal inf();                  // 单位无穷大 inf^1
Hyperreal eps();                  // 单位无穷小 inf^-1
Hyperreal make(double c, int e);  // 指定系数与指数的单项

// ============================================================
//  微积分算法（自由函数风格，类似 STL <algorithm>）
//  - f 采用 std::function<Hyperreal(const Hyperreal&)>，避免按值传参拷贝
//  - x/x0 按 const Hyperreal& 传
// ============================================================
Hyperreal derivative(const std::function<Hyperreal(const Hyperreal&)>& f,
                     const Hyperreal& x,
                     double eps_coef = 1.0);

Hyperreal limit(const std::function<Hyperreal(const Hyperreal&)>& f,
                const Hyperreal& x0,
                int approach = +1);

}  // namespace hyper

#endif
