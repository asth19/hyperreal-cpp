// Hyperreal 超实数库 - 测试程序（16 组用例）
#include "hyper/Hyperreal.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <utility>
#include <cmath>
#include <exception>
using namespace std;
using namespace hyper;


int main()
{
    // 构造三种零：空向量、{{0,0}}、0.0
    Hyperreal empty;                                      // 空
    Hyperreal zero_vec(vector<pair<double,int>>{{0,0}});  // {{0,0}} → 构造后应变空
    Hyperreal zero_d(0.0);                                // 0.0 → 构造后应变空
    Hyperreal normal(vector<pair<double,int>>{{3,0},{1,-1}}); // 3+inf^(-1)

    cout<<"========== 1. 构造规范化 =========="<<endl;
    cout<<"empty.size()  = "<<empty.size()<<"   (期望 0)"<<endl;
    cout<<"zero_vec.size()= "<<zero_vec.size()<<"   (期望 0)"<<endl;
    cout<<"zero_d.size()  = "<<zero_d.size()<<"   (期望 0)"<<endl;
    cout<<"normal.size() = "<<normal.size()<<"   (期望 2)"<<endl;

    // merge_sorted：构造含同指数项的向量，构造归一化（内部走 sort_down + merge_sorted）
    // 应合并为 3*inf^2 + 3*inf^0 + 9*inf^-1
    Hyperreal dup(vector<pair<double,int>>{{1,2},{2,2},{3,0},{4,-1},{5,-1}});
    cout<<"dup 归一化    = "; dup.print();
    cout<<"dup.size()    = "<<dup.size()<<"   (期望 3)"<<endl;
    // 有序输入下 merge 与 merge_sorted 结果一致；对已合并数据两者均为空操作
    Hyperreal dup_m = dup;        dup_m.merge();
    Hyperreal dup_ms = dup;       dup_ms.merge_sorted();
    cout<<"merge == merge_sorted : "<<(dup_m == dup_ms)<<"   (期望 1)"<<endl;
    cout<<"merge_sorted 幂等     : "<<(dup_ms == dup)<<"   (期望 1)"<<endl;

    cout<<"\n========== 2. print 空向量 =========="<<endl;
    cout<<"empty:    "; empty.print();
    cout<<"zero_vec: "; zero_vec.print();
    cout<<"zero_d:   "; zero_d.print();
    cout<<"normal:   "; normal.print();

    cout<<"\n========== 3. get_exp / get_max / print_max =========="<<endl;
    cout<<"empty.get_exp()   = "<<empty.get_exp()<<"   (期望 0)"<<endl;
    cout<<"empty.get_max()   = {"<<empty.get_max().first<<","<<empty.get_max().second<<"}   (期望 {0,0})"<<endl;
    cout<<"empty.print_max() = "; empty.print_max();
    cout<<"normal.get_exp()  = "<<normal.get_exp()<<"   (期望 0)"<<endl;
    cout<<"normal.print_max()= "; normal.print_max();

    cout<<"\n========== 4. 相等性判断 =========="<<endl;
    cout<<"empty == zero_vec : "<<(empty==zero_vec)<<"   (期望 1)"<<endl;
    cout<<"empty == zero_d   : "<<(empty==zero_d)<<"   (期望 1)"<<endl;
    cout<<"zero_vec == zero_d: "<<(zero_vec==zero_d)<<"   (期望 1)"<<endl;
    cout<<"empty == 0.0      : "<<(empty==0.0)<<"   (期望 1)"<<endl;
    cout<<"empty == normal   : "<<(empty==normal)<<"   (期望 0)"<<endl;
    cout<<"empty != normal   : "<<(empty!=normal)<<"   (期望 1)"<<endl;

    cout<<"\n========== 5. 算术运算 =========="<<endl;
    cout<<"empty + normal = "; (empty+normal).print();       // 期望 3+inf^(-1)
    cout<<"normal + empty = "; (normal+empty).print();       // 期望 3+inf^(-1)
    cout<<"empty - normal = "; (empty-normal).print();       // 期望 -3-inf^(-1)
    cout<<"empty * normal = "; (empty*normal).print();       // 期望 0
    cout<<"normal * empty = "; (normal*empty).print();       // 期望 0
    cout<<"5.0 + empty    = "; (5.0+empty).print();          // 期望 5
    cout<<"empty + 5.0    = "; (empty+5.0).print();          // 期望 5
    cout<<"3.0 * empty    = "; (3.0*empty).print();          // 期望 0
    cout<<"empty / 3.0    = "; (empty/3.0).print();          // 期望 0

    cout<<"\n========== 6. 比较运算 =========="<<endl;
    cout<<"normal > empty  : "<<(normal>empty)<<"   (期望 1)"<<endl;
    cout<<"empty < normal  : "<<(empty<normal)<<"   (期望 1)"<<endl;
    cout<<"empty > empty   : "<<(empty>empty)<<"   (期望 0)"<<endl;
    cout<<"empty < empty   : "<<(empty<empty)<<"   (期望 0)"<<endl;
    cout<<"empty >= empty  : "<<(empty>=empty)<<"   (期望 1)"<<endl;
    cout<<"empty <= empty  : "<<(empty<=empty)<<"   (期望 1)"<<endl;
    // 不同指数比较
    Hyperreal hi(vector<pair<double,int>>{{1,2}});       // inf^2
    Hyperreal lo(vector<pair<double,int>>{{3,1}});       // 3*inf^1
    cout<<"inf^2 > 3*inf^1 : "<<(hi>lo)<<"   (期望 1，高次项主导)"<<endl;
    cout<<"3*inf^1 < inf^2 : "<<(lo<hi)<<"   (期望 1)"<<endl;
    // 同指数比较系数
    Hyperreal a5(vector<pair<double,int>>{{5,0}});
    Hyperreal a3(vector<pair<double,int>>{{3,0}});
    cout<<"5 > 3           : "<<(a5>a3)<<"   (期望 1)"<<endl;
    cout<<"3 < 5           : "<<(a3<a5)<<"   (期望 1)"<<endl;
    cout<<"5 > 5           : "<<(a5>a5)<<"   (期望 0)"<<endl;
    // 负数
    Hyperreal neg(vector<pair<double,int>>{{-3,0}});
    cout<<"0 > -3          : "<<(empty>neg)<<"   (期望 1)"<<endl;
    cout<<"-3 < 0          : "<<(neg<empty)<<"   (期望 1)"<<endl;
    cout<<"-3 < 5          : "<<(neg<a5)<<"   (期望 1)"<<endl;
    // 负高次项
    Hyperreal neghi(vector<pair<double,int>>{{-1,2}});   // -inf^2
    cout<<"-inf^2 < 3*inf^1: "<<(neghi<lo)<<"   (期望 1，负高次项更小)"<<endl;
    cout<<"3*inf^1 > -inf^2: "<<(lo>neghi)<<"   (期望 1)"<<endl;

    cout<<"\n========== 7. 数学函数 =========="<<endl;
    cout<<"exp(empty,4)  = "; empty.exp(4).print();          // 期望 1 (e^0=1)
    cout<<"cos(empty,4)  = "; empty.cos(4).print();          // 期望 1 (cos0=1)
    cout<<"sin(empty,4)  = "; empty.sin(4).print();          // 期望 0 (sin0=0)
    cout<<"ln(empty,4)   = "; empty.ln(4).print();           // 期望 [Hyperreal E0211] ...
    cout<<"inv(empty,4)  = "; empty.inv(4).print();          // 期望 [Hyperreal E0101] ...

    cout<<"\n---------- 三角函数测试 ----------"<<endl;
    // sin(π/2+ε) = cos(ε) ≈ 1 - ε?/2
    double PI=3.14159265358979323846;
    Hyperreal halfpi(vector<pair<double,int>>{{PI/2,0},{1,-1}});
    cout<<"sin(π/2+inf^-1,4) = "; halfpi.sin(4).print();
    // cos(π+ε) = -cos(ε) ≈ -(1 - ε?/2)
    Hyperreal piplus(vector<pair<double,int>>{{PI,0},{1,-1}});
    cout<<"cos(π+inf^-1,4)   = "; piplus.cos(4).print();
    // sin(0+ε) = ε - ε?/6
    Hyperreal small(vector<pair<double,int>>{{1,-1}});
    cout<<"sin(inf^-1,4)     = "; small.sin(4).print();
    cout<<"cos(inf^-1,4)     = "; small.cos(4).print();
    // 纯常数，周期归约
    cout<<"sin(1,4)          = "; Hyperreal(1.0).sin(4).print();   // sin(1)≈0.8415
    cout<<"cos(100,4)        = "; Hyperreal(100.0).cos(4).print(); // cos(100归约后)
    // 无穷大报错
    cout<<"sin(inf^2,4)      = "; Hyperreal(vector<pair<double,int>>{{1,2}}).sin(4).print();
    cout<<"cos(inf^2,4)      = "; Hyperreal(vector<pair<double,int>>{{1,2}}).cos(4).print();

    // tan / sinh / cosh / tanh / abs
    cout<<"\n---------- 双曲函数 & abs ----------"<<endl;
    Hyperreal tiny = eps();                                        // inf^-1
    cout<<"tan(inf^-1,4)     = "; tiny.tan(4).print();              // tan(eps) ≈ eps
    cout<<"sinh(inf^-1,4)    = "; tiny.sinh(4).print();            // sinh(eps) ≈ eps
    cout<<"cosh(inf^-1,4)    = "; tiny.cosh(4).print();             // cosh(eps) ≈ 1 + eps^2/2
    cout<<"tanh(inf^-1,4)    = "; tiny.tanh(4).print();             // tanh(eps) ≈ eps
    cout<<"sinh(1,4)         = "; Hyperreal(1.0).sinh(4).print();  // sinh(1) ≈ 1.1752
    cout<<"cosh(1,4)         = "; Hyperreal(1.0).cosh(4).print();  // cosh(1) ≈ 1.5431
    cout<<"tanh(1,4)         = "; Hyperreal(1.0).tanh(4).print();  // tanh(1) ≈ 0.7616
    cout<<"abs(-3+inf^-1)   = "; (-normal).abs().print();         // 3 + inf^-1
    cout<<"abs(3-2*inf^-1)  = "; (Hyperreal(3.0) - make(2,-1)).abs().print(); // 3 - 2*inf^-1（正数保持符号）
    cout<<"tan/sinh/cosh(inf^2) = "; Hyperreal(vector<pair<double,int>>{{1,2}}).tan(4).print();

    cout<<"\n========== 8. 正常向量函数 =========="<<endl;
    cout<<"ln(3+inf^-1)  = "; normal.ln(4).print();
    cout<<"inv(3+inf^-1) = "; normal.inv(4).print();
    cout<<"exp(3+inf^-1,4)= "; normal.exp(4).print();        // e^3 * e^(inf^-1)
    cout<<"exp(inf^-1,4) = "; Hyperreal(vector<pair<double,int>>{{1,-1}}).exp(4).print(); // 纯无穷小
    cout<<"exp(5,4)      = "; Hyperreal(5.0).exp(4).print(); // 纯常数 e^5
    cout<<"exp(inf^2,...) = "; Hyperreal(vector<pair<double,int>>{{1,2}}).exp(4).print(); // HERR_LOG 报错

    cout<<"\n========== 9. 幂运算 =========="<<endl;
    cout<<"empty.pow(0) = "; empty.pow(0).print();           // 期望 1
    cout<<"empty.pow(3) = "; empty.pow(3).print();           // 期望 0
    cout<<"normal.pow(0)= "; normal.pow(0).print();          // 期望 1
    // 通用幂运算
    cout<<"normal.pow(2)= "; normal.pow(2).print();          // (3+inf^-1)^2
    cout<<"Hyperreal(2).pow(3)= "; Hyperreal(2.0).pow(3).print(); // 2^3=8
    // 有理数指数
    cout<<"Hyperreal(4).pow(0.5,5)= "; Hyperreal(4.0).pow(0.5,5).print(); // sqrt(4)=2
    cout<<"Hyperreal(8).pow(1.0/3,5)= "; Hyperreal(8.0).pow(1.0/3,5).print(); // cbrt(8)=2
    // 底数为 1：x^b = 1 对任意 b 成立（ln(1)=0 的特例）
    cout<<"Hyperreal(1).pow(0.5,5) = "; Hyperreal(1.0).pow(0.5,5).print();  // 1^0.5=1
    cout<<"Hyperreal(1).pow(2,5)   = "; Hyperreal(1.0).pow(2.0,5).print();  // 1^2=1
    cout<<"Hyperreal(1).pow(eps,5) = "; Hyperreal(1.0).pow(eps(),5).print();// 1^eps=1
    // (1+inf^-1)^(1/inf^-1) → e
    Hyperreal one_eps(vector<pair<double,int>>{{1,0},{1,-1}});
    Hyperreal inv_eps(vector<pair<double,int>>{{1,-1}});
    cout<<"(1+inf^-1)^(1/inf^-1) ≈ e: "; one_eps.pow(inv_eps.inv(5),5).print();

    // 快速幂正确性：(1+eps)^10 = Σ C(10,k)·eps^k，系数应精确为 1,10,45,120,210,252,...
    Hyperreal p10 = (Hyperreal(1.0) + eps()).pow(10);
    cout<<"(1+eps)^10 = "; p10.print();
    cout<<"           (期望 1 + 10eps + 45eps^2 + 120eps^3 + ... + eps^10，共 11 项)"<<endl;
    cout<<"p10.size() = "<<p10.size()<<"   (期望 11)"<<endl;
    // 幂次复合一致性：(x^3)^2 == x^6
    cout<<"(2^3)^2 == 2^6 : "
        <<(Hyperreal(2.0).pow(3).pow(2) == Hyperreal(2.0).pow(6))
        <<"   (期望 1)"<<endl;

    cout<<"\n========== 10. 链式运算 =========="<<endl;
    // (empty + normal) * 2 - empty == normal * 2
    Hyperreal chain = (empty + normal) * 2.0 - empty;
    cout<<"(empty+normal)*2 - empty = "; chain.print();      // 期望 6+2*inf^(-1)
    cout<<"normal*2                = "; (normal*2.0).print(); // 期望 6+2*inf^(-1)
    cout<<"chain == normal*2       : "<<(chain==normal*2.0)<<"   (期望 1)"<<endl;

    cout<<"\n========== 11. 数值求值 =========="<<endl;
    // exp(inf^-1, 5) 展开后 eval(100) 应接近 e^0.01
    Hyperreal exp_eps=eps().exp(5);
    cout<<"exp(inf^-1,5) = "; exp_eps.print();
    cout<<"eval(100)     = "<<exp_eps.eval(100)<<"   (期望 ≈ "<<std::exp(0.01)<<")"<<endl;
    cout<<"eval(1000)    = "<<exp_eps.eval(1000)<<"   (期望 ≈ "<<std::exp(0.001)<<")"<<endl;
    // sin(inf^-1, 5) eval(100) 应接近 sin(0.01)
    Hyperreal sin_eps=eps().sin(5);
    cout<<"sin(inf^-1,5) = "; sin_eps.print();
    cout<<"eval(100)     = "<<sin_eps.eval(100)<<"   (期望 ≈ "<<std::sin(0.01)<<")"<<endl;
    // 纯常数 eval
    cout<<"Hyperreal(5).eval(999) = "<<Hyperreal(5.0).eval(999)<<"   (期望 5)"<<endl;

    cout<<"\n========== 12. 辅助构造与类型判断 =========="<<endl;
    Hyperreal inf_val=inf();
    Hyperreal eps_val=eps();
    Hyperreal term_val=make(3,-2);
    cout<<"make_inf()    = "; inf_val.print();           // inf^1
    cout<<"make_eps()    = "; eps_val.print();           // inf^-1
    cout<<"make(3,-2)    = "; term_val.print();          // 3*inf^-2
    cout<<"\n--- 类型判断 ---"<<endl;
    cout<<"empty.is_zero()           = "<<empty.is_zero()<<"   (期望 1)"<<endl;
    cout<<"empty.is_real()           = "<<empty.is_real()<<"   (期望 1)"<<endl;
    cout<<"Hyperreal(5).is_real()    = "<<Hyperreal(5.0).is_real()<<"   (期望 1)"<<endl;
    cout<<"normal.is_real()          = "<<normal.is_real()<<"   (期望 0)"<<endl;
    cout<<"inf_val.is_infinite()     = "<<inf_val.is_infinite()<<"   (期望 1)"<<endl;
    cout<<"normal.is_infinite()      = "<<normal.is_infinite()<<"   (期望 0)"<<endl;
    cout<<"eps_val.is_infinitesimal()="<<eps_val.is_infinitesimal()<<"   (期望 1)"<<endl;
    cout<<"normal.is_infinitesimal()  = "<<normal.is_infinitesimal()<<"   (期望 0)"<<endl;
    cout<<"empty.is_infinitesimal()   = "<<empty.is_infinitesimal()<<"   (期望 0)"<<endl;

    // ============================================================
    //  13. 标准化错误处理专项演示
    //  - 错误码对照、HERR_LOG / HERR_SILENT / HERR_THROW 三种策略
    //  - try/catch hyp_exception 并提取结构化信息
    // ============================================================
    cout<<"\n========== 13. 标准化错误处理（策略切换）=========="<<endl;
    cout<<"当前错误策略默认值 : HERR_LOG (1)  实际="<<error_policy()<<endl;
    cout<<"\n--- (a) HERR_LOG：结构化打印到 stderr（cerr）---"<<endl;
    set_error_policy(HERR_LOG);
    cout<<"调用 Hyperreal() / 0.0 :\n  返回值 = ";
    (empty / 0.0).print();

    cout<<"\n--- (b) HERR_SILENT：静默，只返回零值 ---"<<endl;
    set_error_policy(HERR_SILENT);
    cout<<"empty / 0.0 不再打印日志，直接返回 : ";
    (empty / 0.0).print();

    cout<<"\n--- (c) HERR_THROW：抛出 hyp_exception，调用方 try/catch ---"<<endl;
    set_error_policy(HERR_THROW);

    auto run_case=[](const char* label, ErrorCode expect, auto&& action){
        cout<<"\n  ["<<label<<"]  期望错误码 E"
            <<setw(4)<<setfill('0')<<uppercase<<hex<<expect<<dec<<endl;
        try {
            Hyperreal r = action();
            cout<<"    正常返回 : "; r.print();
        } catch (const hyp_exception& ex) {
            cout<<"    catch hyp_exception:"<<endl;
            cout<<"      what   = "<<ex.what()<<endl;
            cout<<"      code   = "<<ex.code_str()<<" (0x"
                <<hex<<(int)ex.code()<<dec<<")"<<endl;
            cout<<"      func   = "<<ex.func()<<endl;
            cout<<"      msg    = "<<ex.msg()<<endl;
            if(!ex.detail().empty()) cout<<"      detail = "<<ex.detail()<<endl;
            cout<<"      匹配期望? "
                <<(ex.code()==expect ? "YES ✅" : "NO ❌")<<endl;
        } catch (const exception& ex) {
            cout<<"    catch std::exception: "<<ex.what()<<endl;
        }
    };

    run_case("empty.ln(4)          ", HRERR_LN_ZERO,          [&]{ return empty.ln(4); });
    run_case("empty.inv(4)         ", HRERR_DIVIDE_BY_ZERO,   [&]{ return empty.inv(4); });
    run_case("empty / 0.0          ", HRERR_DIVIDE_BY_ZERO,   [&]{ return empty / 0.0; });
    run_case("Hyperreal().pow(-2)  ", HRERR_NEG_INT_POWER,    [&]{ return empty.pow((signed int)-2); });
    run_case("inf_val.exp(4)       ", HRERR_EXP_INFINITY,     [&]{ return inf_val.exp(4); });
    run_case("inf_val.sin(4)       ", HRERR_SIN_INFINITY,     [&]{ return inf_val.sin(4); });
    run_case("inf_val.cos(4)       ", HRERR_COS_INFINITY,     [&]{ return inf_val.cos(4); });
    run_case("Hyperreal(-3).ln(4)  ", HRERR_LN_NEGATIVE,      [&]{ return Hyperreal(-3.0).ln(4); });
    run_case("empty.pow(Halfpi, 4) ", HRERR_ZERO_NEG_POWER,
            [&]{ return empty.pow( Hyperreal( vector<pair<double,int>>{{-1,0}} ), 4 ); });

    cout<<"\n--- (d) error_message 翻译完整性（新增错误码不能回落为未知错误）---"<<endl;
    cout<<"E0223 tan  : "<<error_message(HRERR_TAN_INFINITY)<<"   (期望 tan(无穷大)无定义)"<<endl;
    cout<<"E0231 sinh : "<<error_message(HRERR_SINH_INFINITY)<<"   (期望 sinh(无穷大)无定义)"<<endl;
    cout<<"E0232 cosh : "<<error_message(HRERR_COSH_INFINITY)<<"   (期望 cosh(无穷大)无定义)"<<endl;
    cout<<"E0233 tanh : "<<error_message(HRERR_TANH_INFINITY)<<"   (期望 tanh(无穷大)无定义)"<<endl;

    // 恢复默认策略
    set_error_policy(HERR_LOG);

    // ============================================================
    //  14. 精度控制：set_max_terms / truncated / truncated_by_exp
    // ============================================================
    cout<<"\n========== 14. 精度控制 =========="<<endl;
    cout<<"默认 max_terms = "<<(max_terms().has_value() ? to_string(max_terms().value()) : "unlimited")<<endl;

    // 构造一个高精度无穷小级数：(1+eps)^20 展开后会有很多项
    Hyperreal base = Hyperreal(1.0) + eps();
    Hyperreal expanded = base.pow(20);
    cout<<"\n(1+eps)^20 展开后的项数: "<<expanded.size()<<endl;

    // 设置全局上限，再看 pow 后是否被截断
    set_max_terms(5);
    Hyperreal expandedLimited = base.pow(20);
    cout<<"\n设置 max_terms=5 后，(1+eps)^20 的项数: "<<expandedLimited.size()<<endl;
    // 恢复不限制
    clear_max_terms();

    // truncated(n) 按项数截断
    Hyperreal tr = expanded.truncated(3);
    cout<<"\nexpanded.truncated(3) 后项数: "<<tr.size()<<endl;
    cout<<"truncated 前 3 项 = "; tr.print(3);

    // truncated_by_exp(min, max) 按指数区间截断
    Hyperreal tr_exp = expanded.truncated_by_exp(-3, 0);
    cout<<"\nexpanded.truncated_by_exp(-3, 0) 后项数: "<<tr_exp.size()<<endl;
    cout<<"保留指数 [-3,0] 的项 = "; tr_exp.print(4);

    // ---- 全局指数区间截断 ----
    cout<<"\n--- 全局指数区间截断 set_exp_range ---"<<endl;
    auto er_min = exp_range_min();
    auto er_max = exp_range_max();
    cout<<"默认 exp_range = ["<<(er_min.has_value() ? to_string(er_min.value()) : "unlimited")
        <<", "<<(er_max.has_value() ? to_string(er_max.value()) : "unlimited")<<"]"<<endl;

    // 只保留实数部分：set_exp_range(0, 0)
    Hyperreal big = Hyperreal(container{{1,2}, {3,1}, {5,0}, {7,-1}, {9,-2}});
    cout<<"big 原始 = "; big.print();
    set_exp_range(0, 0);
    Hyperreal only_real = Hyperreal(container{{1,2}, {3,1}, {5,0}, {7,-1}, {9,-2}});
    cout<<"set_exp_range(0,0) 后构造 = "; only_real.print();
    // 验证算术运算也受影响
    set_exp_range(-1, 1);
    Hyperreal clipped = Hyperreal(container{{1,2}, {3,1}, {5,0}, {7,-1}, {9,-2}});
    cout<<"set_exp_range(-1,1) 后构造 = "; clipped.print();
    cout<<"clipped.size() = "<<clipped.size()<<"  (期望 3)"<<endl;
    // 恢复不限制
    clear_exp_range();
    auto er_min2 = exp_range_min();
    auto er_max2 = exp_range_max();
    cout<<"恢复后 exp_range = ["<<(er_min2.has_value() ? to_string(er_min2.value()) : "unlimited")
        <<", "<<(er_max2.has_value() ? to_string(er_max2.value()) : "unlimited")<<"]"<<endl;

    // ============================================================
    //  15. 分量提取（非标准分析）
    // ============================================================
    cout<<"\n========== 15. 分量提取 =========="<<endl;
    // h = 3 + 2*inf - 5*inf^-1 + 7*inf^-2
    Hyperreal h = Hyperreal(vector<pair<double,int>>{
        {3,0}, {2,1}, {-5,-1}, {7,-2}
    });
    cout<<"原始 h        = "; h.print();
    cout<<"standard_part  = "; h.standard_part().print();        // 期望 3 + 2*inf
    cout<<"real_part      = "; h.real_part().print();             // 期望 3
    cout<<"infinite_part  = "; h.infinite_part().print();         // 期望 2*inf
    cout<<"infinitesimal_part = "; h.infinitesimal_part().print(); // 期望 -5*inf^-1 + 7*inf^-2
    cout<<"principal_term = "; h.principal_term().print();        // 期望 2*inf

    // ============================================================
    //  16. 微积分
    // ============================================================
    cout<<"\n========== 16. 微积分 =========="<<endl;

    // (a) 求导：f(x) = x^2，f'(2) 应为 4
    auto f_square = [](const Hyperreal& x) -> Hyperreal { return x.pow(2); };
    Hyperreal d_at_2 = derivative(f_square, Hyperreal(2.0));
    cout<<"[求导]  f(x)=x^2  →  f'(2) = "; d_at_2.print();
    cout<<"           (期望 4)"<<endl;

    // (b) 求导：f(x) = exp(x)，f'(0) 应为 1
    auto f_exp = [](const Hyperreal& x) -> Hyperreal { return x.exp(8); };
    Hyperreal d_at_0 = derivative(f_exp, Hyperreal(0.0));
    cout<<"[求导]  f(x)=exp(x) →  f'(0) = "; d_at_0.print();
    cout<<"           (期望 1)"<<endl;

    // (c) 极限：lim_{x→0} sin(x)/x = 1
    auto f_sinx_over_x = [](const Hyperreal& x) -> Hyperreal {
        if(x.is_zero()) return Hyperreal(1.0);  // 0/0 的占位
        return x.sin(8) / x;
    };
    Hyperreal lim_sinx = limit(f_sinx_over_x, Hyperreal(0.0));
    cout<<"[极限]  lim_{x→0} sin(x)/x = "; lim_sinx.print();
    cout<<"           (期望 1)"<<endl;

    // (d) 极限：lim_{x→0} (1-cos(x))/x^2 = 1/2
    auto f_1mc_over_x2 = [](const Hyperreal& x) -> Hyperreal {
        if(x.is_zero()) return Hyperreal(0.5);
        return (Hyperreal(1.0) - x.cos(8)) / (x * x);
    };
    Hyperreal lim_cos = limit(f_1mc_over_x2, Hyperreal(0.0));
    cout<<"[极限]  lim_{x→0} (1-cos(x))/x^2 = "; lim_cos.print();
    cout<<"           (期望 0.5)"<<endl;

    // 极限：lim_{x→inf} x*sin(1/x) = 1
    auto f_xsin_over_x = [](const Hyperreal& x) -> Hyperreal {
        if(x.is_zero()) return Hyperreal(1.0);
        return x * x.inv(2).sin(2) ;
    };
    Hyperreal lim_xsin_over_x = limit(f_xsin_over_x, inf());
    cout<<"[极限]  lim_{x→inf} x*sin(1/x) = "; lim_xsin_over_x.print();
    cout<<"           (期望 1)"<<endl;

    // 复杂极限：lim_{n→∞} n²[(1+1/(n+1))^(n+1) - (1+1/n)^n] = e/2
    // 由 (1+1/n)^n = e(1 - 1/(2n) + 11/(24n²) - 7/(16n³) - ...)，
    // 两项相减后 e 与 -e/(2n) 完全抵消，首项为 e/(2n²)，故 n²·差 → e/2。
    // 该极限同时考验：无穷大指数幂 pow、inv 级数、高阶相消后的首项精度。
    auto f_e_half = [](const Hyperreal& n) -> Hyperreal {
        Hyperreal a = (Hyperreal(1.0) + (n + 1).inv(3)).pow(n + 1, 3);
        Hyperreal b = (Hyperreal(1.0) + n.inv(3)).pow(n, 3);
        return (a - b) * (n * n);
    };
    Hyperreal lim_e_half = limit(f_e_half, inf());
    cout<<"[极限]  lim_{n→∞} n²[(1+1/(n+1))^(n+1) - (1+1/n)^n] = "; lim_e_half.print(4);
    cout<<"           (期望 e/2 ≈ 1.35914)"<<endl;

    // (e) 连续性判定：f(x)=x^2 在 x=2 连续（diff 为无穷小）
    cout<<"\n[连续]  f(x)=x^2 在 x=2 连续? "
        <<(is_continuous(f_square, Hyperreal(2.0)) ? "YES" : "NO")<<endl;
    cout<<"           (期望 YES)"<<endl;

    // (f) 连续性判定：离散函数 f(0)=0, f(x≠0)=1 在 x=0 不连续（diff = 1，非无穷小）
    auto f_discrete = [](const Hyperreal& x) -> Hyperreal {
        return x.is_zero() ? Hyperreal(0.0) : Hyperreal(1.0);
    };
    cout<<"[连续]  f(0)=0, f(x!=0)=1 在 x=0 连续? "
        <<(is_continuous(f_discrete, Hyperreal(0.0)) ? "YES" : "NO")<<endl;
    cout<<"           (期望 NO，离散函数在 0 处跳跃)"<<endl;

    return 0;
}
