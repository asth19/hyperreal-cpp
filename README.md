# Hyperreal 超实数库

基于非标准分析（Nonstandard Analysis）的 C++ 超实数（Hyperreal）运算库。将无穷大 `inf`  作为一阶对象纳入运算体系，支持符号级展开、求导、求极限等数学任务。

## 数学背景

### 非标准分析与超实数系

1960 年代，Abraham Robinson 创立**非标准分析**（Nonstandard Analysis），用严格逻辑把无穷大与无穷小重新纳入数学分析，避开 ε-δ 语言的繁琐。其核心是把实数系 R 扩张为**超实数系 \*R**，其中包含三类数：

- **无穷大**（infinite numbers）：绝对值大于任何正实数的数，如本库的 `inf`
- **无穷小**（infinitesimals）：绝对值小于任何正实数但非零的数，如 `eps = inf^-1`
- **有限超实数**：形如 `实数 + 无穷小` 的数，如实数 3 附近的超实数 `3 + eps`

### 转移原则

非标准分析的核心定理：标准实数 R 中成立的任何一阶命题，在超实数系 \*R 中同样成立。这保证超实数运算继承实数运算的代数性质——加法交换律、乘法分配律、三角恒等式等均自动生效，无需重新证明。

### 标准部分与微积分定义

任何有限超实数 x 可唯一分解为 `x = r + h`，其中 r 为实数（标准部分 standard part），h 为无穷小。记 `st(x) = r`。基于此，微积分的定义变得直观：

| 概念 | 非标准定义 | 本库对应 API |
|------|-----------|-------------|
| 导数 | `f'(x) = st( (f(x+eps) - f(x)) / eps )` | `derivative(f, x)` |
| 极限 | `lim_{t→x} f(t) = L  ⟺  st(f(x+eps)) = L` | `limit(f, x)` |
| 连续 | `f` 在 x 连续 ⟺ `f(x+eps) - f(x)` 为无穷小 | 间接由 `limit` 判定 |
| 标准部分 | `st(a + h) = a` | `standard_part()` |

### 本库的表示：形式幂级数

本库不引入任意无穷小，而采用受限但可计算的形式幂级数表示：每个超实数表示为以 `inf` 为不定元的多项式：

```
a_0 * inf^e_0 + a_1 * inf^e_1 + ... + a_n * inf^e_n
```

其中指数 e_i 为整数（可正可负），系数 a_i 为非零实数，按指数降序存储。例：

- `3*inf^0 + 1*inf^-1` — 实数 3 加单位无穷小
- `1*inf^1` — 单位无穷大
- `1 + 2*inf^-1 + 3*inf^-2` — `1/(1-2*eps)` 的幂级数展开前 3 项

运算规则：

- **加法**：同指数项系数相加，消去零系数项
- **乘法**：项式相乘后合并同指数项
- **类型判定**：最高指数 > 0 为无穷大；< 0 为无穷小；≤ 0 且含指数 = 0 项为有限超实数
- **标准部分**：丢弃所有指数 < 0 的项
- **初等函数**：`exp` / `ln` / `sin` / `cos` / `inv` 等在标准部分处作泰勒展开，生成无穷小多项式

这种表示不能覆盖所有超实数（如任意无穷序列的无穷小复合），但足以处理微积分与初等函数运算的常见情形，且展开结果符号化、可读、误差可量化（见下方 [精度控制](#精度控制)）。

---

## 特性

- **超实数表示**：以 `系数*inf^指数` 的项序列存储，自动合并、排序、去零
- **完整算术与比较**：`+ - * /`、`== != > < >= <=`，支持超实数与 `double` 混合运算
- **数学函数**：`pow / exp / ln / sin / cos / inv`（基于级数展开，精度可控）
- **微积分算法**：`derivative`（求导）、`limit`（极限，支持 `x→x0` 与 `x→inf`）、`is_continuous`（连续性判定）
- **分量提取**：`standard_part` / `real_part` / `infinite_part` / `infinitesimal_part` / `principal_term`
- **精度控制**：`set_max_terms` 截断项数、`truncated` 按项数截断、`truncated_by_exp` 按指数区间截断
- **三档错误处理**：`HERR_SILENT`（静默）/ `HERR_LOG`（日志，默认）/ `HERR_THROW`（抛异常）
- **结构化异常**：`hyp_exception` 继承 `std::exception`，携带错误码 + 函数名 + 上下文详情
- **CMake 构建**：编译为静态库 `libhyperreal`，可被外部项目复用

## 目录结构

```
超实数库/
├── include/hyper/Hyperreal.h     # 公共头文件（namespace hyper）
├── src/Hyperreal.cpp             # 库实现
├── test/test_hyperreal.cpp       # 测试程序（16 组用例）
├── CMakeLists.txt                # 库 + 测试双 target
├── build_and_run.ps1             # 一键编译运行脚本（PowerShell）
├── build_and_run.cmd             # 双击入口（调用 ps1）
└── .gitignore
```

## 构建方法

### 依赖

- CMake ≥ 3.15
- Ninja（推荐）或 Make
- GCC/MinGW（g++ ≥ 8，需 C++17）或 MSVC

### 命令行构建

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

产物：

- `build/libhyperreal.a`（静态库）
- `build/bin/test_hyperreal.exe`（测试程序）

### 一键脚本（Windows）

右键 `build_and_run.ps1` → "使用 PowerShell 运行"，或双击 `build_and_run.cmd`。脚本会检查工具链、配置 CMake、编译并运行测试。

## 快速上手

### 在自己的项目中使用

```cmake
# CMakeLists.txt
add_subdirectory(超实数库)        # 或安装后用 find_package
target_link_libraries(your_app PRIVATE hyperreal)
```

```cpp
#include "hyper/Hyperreal.h"
using namespace hyper;

int main() {
    Hyperreal x(3.0);                 // 实数 3
    Hyperreal e = eps();               // 无穷小 inf^-1
    Hyperreal inf_val = inf();        // 无穷大 inf^1

    Hyperreal y = x + e;               // 3 + 无穷小
    y.print();                        // 3*inf^0 + 1*inf^-1

    Hyperreal s = (Hyperreal(1.0) + e).exp(8);   // e^(1+eps) 的级数展开
    s.print();
    return 0;
}
```

### 构造与运算

```cpp
// 三种构造方式
Hyperreal empty;                                            // 0（空向量）
Hyperreal from_vec(std::vector<std::pair<double,int>>{{3,0},{1,-1}});  // 3+eps
Hyperreal from_double(5.0);                                 // 实数 5
Hyperreal custom = make(2, -3);                             // 2*inf^-3

// 算术（支持与 double 混合）
Hyperreal a = from_vec + custom;     // 加法
Hyperreal b = 2.0 * from_vec;        // 左操作数 double
Hyperreal c = from_vec * 3.0;        // 右操作数 double
```

### 大小比较

比较按"最高指数项主导"原则：先比最高指数项的指数，指数高者大；指数相同则比系数。

```cpp
using namespace hyper;

// 不同指数：高次项主导
Hyperreal hi = make(1, 2);      // inf^2
Hyperreal lo = make(3, 1);      // 3*inf^1
(hi > lo);                      // true，inf^2 量级大于 3*inf

// 同指数：比较系数
Hyperreal a5 = make(5, 0);      // 5
Hyperreal a3 = make(3, 0);      // 3
(a5 > a3);                      // true

// 负数与负高次项
Hyperreal neg   = make(-3, 0);  // -3
Hyperreal neghi = make(-1, 2);  // -inf^2
(neg < a3);                     // true
(neghi < lo);                   // true，负的高次项比任何正项都小

// 与 double 比较（支持混合）
(Hyperreal(5.0) > 3.0);         // true
```

### 数学函数

```cpp
Hyperreal x = make(3, 0) + eps();    // 3 + inf^-1

x.ln(6).print();    // ln(3+eps) 泰勒展开
x.exp(4).print();   // exp(3+eps) 泰勒展开
x.inv(5).print();   // 1/(3+eps) 泰勒展开
x.sin(4).print();   // sin(3+eps)
x.cos(4).print();   // cos(3+eps)
x.pow(2).print();   // (3+eps)^2 = 9 + 6*eps + eps^2
```

### 微积分：求导与极限

```cpp
// 求导：f(x) = x^2，f'(2) = 4
auto f_square = [](const Hyperreal& x) -> Hyperreal { return x.pow(2); };
Hyperreal d = derivative(f_square, Hyperreal(2.0));
d.print();    // 4*inf^0 + 1*inf^-1

// 极限：lim_{x→0} sin(x)/x = 1
auto f_sinx_over_x = [](const Hyperreal& x) -> Hyperreal {
    if (x.is_zero()) return Hyperreal(1.0);    // 0/0 占位
    return x.sin(8) / x;
};
Hyperreal lim = limit(f_sinx_over_x, Hyperreal(0.0));
lim.print();   // 1*inf^0 + -0.166667*inf^-2 + ...

// 极限：lim_{x→inf} x*sin(1/x) = 1
auto f_xsin_over_x = [](const Hyperreal& x) -> Hyperreal {
    if (x.is_zero()) return Hyperreal(1.0);
    return x * x.inv(3).sin(3);
};
Hyperreal lim_inf = limit(f_xsin_over_x, inf());
lim_inf.print();  // 1*inf^0 + ...
```

### 分量提取

```cpp
Hyperreal h = make(2, 1) + make(3, 0) + make(-5, -1) + make(7, -2);
// h = 2*inf^1 + 3*inf^0 + -5*inf^-1 + 7*inf^-2

h.standard_part().print();       // 2*inf^1 + 3*inf^0  （丢弃无穷小）
h.real_part().print();            // 3*inf^0            （仅常数项）
h.infinite_part().print();        // 2*inf^1            （仅无穷大项）
h.infinitesimal_part().print();   // -5*inf^-1 + 7*inf^-2
h.principal_term().print();       // 2*inf^1            （首项）
```

## API 概览

所有公开符号位于 `namespace hyper`。

### 类型与枚举

| 符号            | 说明                                                                    |
| ------------- | --------------------------------------------------------------------- |
| `value_type`  | `std::pair<double, int>`（系数, 指数）                                      |
| `container`   | `std::vector<value_type>`                                             |
| `ErrorCode`   | 错误码枚举：`HRERR_DIVIDE_BY_ZERO`、`HRERR_LN_ZERO`、`HRERR_SIN_INFINITY` 等   |
| `ErrorPolicy` | 策略枚举：`HERR_SILENT` / `HERR_LOG` / `HERR_THROW`                        |
| `hyp_exception`   | 异常类，继承 `std::exception`，提供 `code()` / `func()` / `msg()` / `detail()` |

### 全局配置（自由函数）

| 函数                                       | 说明                   |
| ---------------------------------------- | -------------------- |
| `set_error_policy(p)` / `error_policy()` | 设置/查询错误处理策略          |
| `set_max_terms(n)` / `max_terms()`       | 设置/查询最大保留项数（0 = 不限制） |
| `error_message(code)`                    | 错误码翻译为默认中文描述         |
| `format_error(code, func, msg, detail)`  | 结构化错误日志格式            |

### `class Hyperreal`

| 分类   | 成员                                                                                                           |
| ---- | ------------------------------------------------------------------------------------------------------------ |
| 构造   | `Hyperreal()` / `Hyperreal(double)` / `Hyperreal(const container&)`（后两者 `explicit`）                          |
| 信息   | `print()` / `print(n)` / `get_coe(exp)` / `get_exp()` / `get_max()` / `get_num()` / `print_max()` / `size()` |
| 类型判断 | `is_zero()` / `is_real()` / `is_infinite()` / `is_infinitesimal()`                                           |
| 格式化  | `merge()` / `sort_up()` / `sort_down()` / `remove0()`                                                        |
| 精度   | `truncated(n)` / `truncated_by_exp(min_exp, max_exp)`                                                  |
| 运算符  | `+ - * /`（含 `double` 与友元版本）、`== != > < >= <=`、`+= -= *= /=`                                                  |
| 数学函数 | `pow(n)` / `pow(b, len)` / `exp(len)` / `ln(len)` / `inv(len)` / `sin(len)` / `cos(len)`                     |
| 求值   | `eval(x)`（代入实数 x 求近似值）                                                                                       |
| 分量提取 | `standard_part()` / `real_part()` / `infinite_part()` / `infinitesimal_part()` / `principal_term()`          |

### 工厂函数

| 函数           | 说明                   |
| ------------ | -------------------- |
| `inf()`      | 单位无穷大 `1*inf^1`      |
| `eps()`      | 单位无穷小 `1*inf^-1`     |
| `make(c, e)` | 指定系数与指数的单项 `c*inf^e` |

### 微积分算法

| 函数                               | 说明                                                                    |
| -------------------------------- | --------------------------------------------------------------------- |
| `derivative(f, x, eps_coef=1.0)` | 求 `f` 在 `x` 处的导数（超实数差商）                                               |
| `limit(f, x0, approach=+1)`      | 求 `f` 在 `x0` 处的极限，`approach` 为逼近方向（+1 右极限，-1 左极限）；`x0=inf()` 时求无穷远处极限 |
| `is_continuous(f, x)`            | 判定 `f` 在 `x` 处是否连续（`f(x+eps)-f(x)` 为无穷小或零则连续） |

## 错误处理

库内置三档策略，通过 `set_error_policy()` 切换，全局生效：

| 策略             | 行为                             | 适用场景          |
| -------------- | ------------------------------ | ------------- |
| `HERR_SILENT`  | 静默返回零值，不打印不抛                   | 批量计算，主动跳过坏项   |
| `HERR_LOG`（默认） | 向 `stderr` 打印结构化错误，返回零值        | 一般调试          |
| `HERR_THROW`   | 抛出 `hyp_exception`，调用方 `try/catch` | 严格场景，要求错误不被吞没 |

### 策略切换示例

```cpp
using namespace hyper;

// 切到抛异常模式
set_error_policy(HERR_THROW);

try {
    Hyperreal zero;
    Hyperreal bad = zero.ln(4);     // ln(0) 无定义
} catch (const hyp_exception& ex) {
    std::cerr << "code: "   << ex.code_str() << "\n"
              << "func: "   << ex.func()     << "\n"
              << "msg: "    << ex.msg()      << "\n"
              << "detail: " << ex.detail()   << "\n";
    // code:   E0211
    // func:   ln
    // msg:    ln(0)无定义
    // detail: 超实数为 0
}

// 切回静默模式
set_error_policy(HERR_SILENT);
Hyperreal result = Hyperreal(0.0).inv(4);   // 直接返回 0，不打印
```

### 错误码一览

| 错误码                             | 含义                      |
| ------------------------------- | ----------------------- |
| `HRERR_DIVIDE_BY_ZERO` (0x0101) | 除零或 `inv(0)`            |
| `HRERR_NEG_INT_POWER` (0x0102)  | `pow(signed int)` 收到负数幂 |
| `HRERR_ZERO_NEG_POWER` (0x0103) | 0 的负次幂                  |
| `HRERR_EXP_INFINITY` (0x0201)   | `exp(无穷大)` 无定义          |
| `HRERR_LN_ZERO` (0x0211)        | `ln(0)` 无定义             |
| `HRERR_LN_NEGATIVE` (0x0212)    | `ln(负数)` 无定义            |
| `HRERR_LN_INFINITY` (0x0213)    | `ln(无穷大)` 无定义           |
| `HRERR_SIN_INFINITY` (0x0221)   | `sin(无穷大)` 无定义          |
| `HRERR_COS_INFINITY` (0x0222)   | `cos(无穷大)` 无定义          |

## 精度控制

级数展开类函数（`exp` / `ln` / `sin` / `cos` / `inv` 等）接受 `len` 参数控制保留项数。全局 `set_max_terms(n)` 可统一限制所有展开操作的最高项数（0 = 不限制），防止 `pow` 等操作无限膨胀向量。

```cpp
using namespace hyper;

Hyperreal e = (Hyperreal(1.0) + eps()).pow(20);   // 理论上 21 项
std::cout << "项数: " << e.size() << "\n";         // 21

set_max_terms(5);
Hyperreal truncated_e = (Hyperreal(1.0) + eps()).pow(20);   // 截断到 5 项
std::cout << "截断后项数: " << truncated_e.size() << "\n"; // 5

// 对已有对象取截断副本
Hyperreal first3 = e.truncated(3);              // 按项数：保留前 3 项 1 + 20*eps + 190*eps^2
Hyperreal band   = e.truncated_by_exp(-3, 0);   // 按指数区间：保留指数 [-3,0] 的项，丢弃无穷大项与更深无穷小
```

## 测试

运行测试程序验证所有功能：

```bash
./build/bin/test_hyperreal.exe
```

测试包含 16 组用例，覆盖构造规范化、算术运算、比较、幂函数、三角函数、链式运算、数值求值、类型判断、错误策略切换、精度控制、分量提取、微积分（求导 + 极限）。

预期输出（节选）：

```
========== 16. 微积分 ==========
[求导]  f(x)=x^2  →  f'(2) = 4*inf^0 + 1*inf^-1
           (期望 4)
[求导]  f(x)=exp(x) →  f'(0) = 1*inf^0 + 0.5*inf^-1 + ...
           (期望 1)
[极限]  lim_{x→0} sin(x)/x = 1*inf^0 + -0.166667*inf^-2 + ...
           (期望 1)
[极限]  lim_{x→0} (1-cos(x))/x^2 = 0.5*inf^0 + ...
           (期望 0.5)
[极限]  lim_{x→inf} x*sin(1/x) = 1*inf^0 + ...
           (期望 1)
```

## 编译选项

| 平台    | 编译选项                                                                                  |
| ----- | ------------------------------------------------------------------------------------- |
| MinGW | `-Wall -Wextra -D__USE_MINGW_ANSI_STDIO=1 -finput-charset=UTF-8 -fexec-charset=UTF-8` |
| MSVC  | `/utf-8 /W4`                                                                          |

C++ 标准：C++17（`CMAKE_CXX_STANDARD 17`，`CMAKE_CXX_EXTENSIONS OFF`）。

