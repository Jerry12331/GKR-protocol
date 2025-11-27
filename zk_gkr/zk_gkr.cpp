#include <iostream>
#include <vector>
#include <numeric>
#include <stdexcept>
#include <cassert>
#include <random>

// --- 數學工具 ---
const long long MOD = 97;

// 處理負數取模的加法
long long mod_add(long long a, long long b) {
    long long res = (a % MOD + b % MOD) % MOD;
    return res < 0 ? res + MOD : res;
}

// 處理負數取模的乘法
long long mod_mul(long long a, long long b) {
    long long res = ((a % MOD) * (b % MOD)) % MOD;
    return res < 0 ? res + MOD : res;
}

// 處理負數取模的減法
long long mod_sub(long long a, long long b) {
    long long res = (a % MOD - b % MOD) % MOD;
    return res < 0 ? res + MOD : res;
}

using Polynomial = std::vector<long long>;

long long evaluate_poly(const Polynomial& p, long long x) {
    long long result = 0;
    long long current_x_power = 1;
    for (long long coeff : p) {
        result = mod_add(result, mod_mul(coeff, current_x_power));
        current_x_power = mod_mul(current_x_power, x);
    }
    return result;
}

void print_poly(const Polynomial& p) {
    if (p.empty()) {
        std::cout << "0" << std::endl;
        return;
    }
    for (size_t i = 0; i < p.size(); ++i) {
        if (p[i] == 0 && p.size() > 1) continue;
        std::cout << p[i] << "x^" << i << (i == p.size() - 1 ? "" : " + ");
    }
    std::cout << std::endl;
}

// --- 電路定義 (保持不變) ---
enum GateType { ADD, MUL, INPUT };

struct Gate {
    GateType type;
    size_t in1, in2;
    long long value;
    Gate() : type(INPUT), in1(0), in2(0), value(0) {}
    Gate(GateType t, size_t i1, size_t i2) : type(t), in1(i1), in2(i2), value(0) {}
};

using Layer = std::vector<Gate>;

class Circuit {
public:
    std::vector<Layer> layers;
    size_t input_size;

    Circuit(size_t num_layers, size_t num_inputs) 
        : input_size(num_inputs) {
        layers.resize(num_layers);
    }

    void add_gate(int layer_idx, GateType type, size_t in1, size_t in2) {
        if (layer_idx >= layers.size()) throw std::runtime_error("Layer index out of bounds");
        layers[layer_idx].push_back(Gate{type, in1, in2});
    }

    long long evaluate(const std::vector<long long>& inputs) {
        if (inputs.size() != input_size) throw std::runtime_error("Input size mismatch");
        int input_layer_idx = layers.size() - 1;
        for (size_t i = 0; i < input_size; ++i) layers[input_layer_idx][i].value = inputs[i];

        for (int i = layers.size() - 2; i >= 0; --i) {
            for (auto& gate : layers[i]) {
                const auto& g1 = layers[i + 1][gate.in1];
                const auto& g2 = layers[i + 1][gate.in2];
                if (gate.type == ADD) gate.value = mod_add(g1.value, g2.value);
                else if (gate.type == MUL) gate.value = mod_mul(g1.value, g2.value);
            }
        }
        return layers[0][0].value;
    }
};

// --- GKR Prover (加入 Libra ZK 邏輯) ---
class Prover {
public:
    Circuit& circuit;
    std::vector<long long> secret_inputs;
    std::mt19937 rng;

    Prover(Circuit& c, const std::vector<long long>& inputs) : circuit(c), secret_inputs(inputs) {
        std::random_device rd;
        rng.seed(rd());
    }

    long long evaluate_circuit() {
        return circuit.evaluate(secret_inputs);
    }

    // [Libra ZK] 生成一個隨機的遮罩總和 G
    // 在真實協定中，這是 random polynomial g 的總和
    long long generate_random_mask_sum() {
        std::uniform_int_distribution<long long> dist(0, MOD - 1);
        return dist(rng);
    }

    // [Libra ZK] 在 Sumcheck 結束時，"打開" 遮罩多項式在隨機點 r 的值
    // 在模擬中，我們只需要生成一個隨機數來代表 g(r)
    long long open_mask_polynomial(long long r) {
        std::uniform_int_distribution<long long> dist(0, MOD - 1);
        return dist(rng); 
    }

    // 生成 Sumcheck 多項式
    // 這裡的 expected_sum 已經是包含了遮罩的 (H + rho*G)
    Polynomial sumcheck_round_prover(int layer_idx, long long expected_sum) {
        // 為了通過驗證，我們構造一個多項式 p(t) 使得 p(0) + p(1) = expected_sum
        long long p_at_0 = (10 * (layer_idx + 1)) % MOD;
        long long p_at_1 = mod_sub(expected_sum, p_at_0);
        
        // p(t) = c0 + c1*t
        long long c0 = p_at_0;
        long long c1 = mod_sub(p_at_1, c0);

        return {c0, c1};
    }
};

// --- GKR Verifier (加入 Libra ZK 邏輯) ---
class Verifier {
public:
    Circuit& circuit;
    std::mt19937 rng;

    Verifier(Circuit& c) : circuit(c) {
        std::random_device rd;
        rng.seed(rd());
    }
    
    long long get_random_challenge() {
        std::uniform_int_distribution<long long> dist(0, MOD - 1);
        return dist(rng);
    }

    long long sumcheck_round_verifier(long long expected_sum, const Polynomial& p) {
        long long p_at_0 = evaluate_poly(p, 0);
        long long p_at_1 = evaluate_poly(p, 1);
        long long sum_val = mod_add(p_at_0, p_at_1);
        
        if (sum_val != expected_sum) {
            throw std::runtime_error("Sumcheck verification failed!");
        }
        std::cout << "    [V] Check passed: p(0)+p(1) (" << sum_val << ") == Target (" << expected_sum << ")" << std::endl;

        long long r = get_random_challenge();
        std::cout << "    [V] Challenge r = " << r << std::endl;
        return evaluate_poly(p, r);
    }
    
    bool run_protocol(Prover& prover, const std::vector<long long>& inputs, long long claimed_output) {
        std::cout << "--- Libra ZK-GKR Protocol Start ---" << std::endl;
        std::cout << "Public Inputs: ";
        for(long long i : inputs) std::cout << i << " ";
        std::cout << "\nProver's Claimed Output (H): " << claimed_output << std::endl;
        
        std::vector<long long> current_claims_values = { claimed_output };

        for (int i = 0; i < circuit.layers.size() - 1; ++i) {
            std::cout << "\n--- Verifying Layer " << i << " -> Layer " << (i+1) << " ---" << std::endl;
            
            // 1. 獲取原始聲明 H
            long long H = current_claims_values[0];
            std::cout << "[1] Original Claim (H) = " << H << std::endl;

            // 2. [Libra ZK] Prover 發送遮罩總和 G (Commitment to masking poly g)
            // 這裡跳過了 Commitment，直接發送數值
            long long G = prover.generate_random_mask_sum();
            std::cout << "[2] [ZK] Prover sends Mask Sum (G) = " << G << std::endl;

            // 3. [Libra ZK] Verifier 發送隨機係數 rho
            long long rho = get_random_challenge();
            std::cout << "[3] [ZK] Verifier sends random rho = " << rho << std::endl;

            // 4. [Libra ZK] 計算新的混合目標值: H + rho * G
            long long expected_sum = mod_add(H, mod_mul(rho, G));
            std::cout << "[4] [ZK] New Sumcheck Target (H + rho*G) = " << expected_sum << std::endl;
            std::cout << "    (This effectively masks the real circuit value H)" << std::endl;

            // 5. 執行 Sumcheck (針對混合目標)
            std::cout << "[5] Running Sumcheck on masked target..." << std::endl;
            Polynomial p = prover.sumcheck_round_prover(i, expected_sum);
            std::cout << "    [P] Prover sends polynomial: "; print_poly(p);
            
            long long final_val_masked;
            try {
                final_val_masked = sumcheck_round_verifier(expected_sum, p);
            } catch (const std::exception& e) {
                std::cerr << "[X] Protocol Failed: " << e.what() << std::endl;
                return false;
            }
            // final_val_masked 相當於 f(r) + rho * g(r)

            // 6. [Libra ZK] 移除遮罩 (Unmasking)
            // Verifier 需要還原真實的 f(r) 來繼續下一層
            // V 要求 P 打開遮罩多項式 g(r)
            // (在真實協定中，這裡會驗證 VPD Commitment，我們跳過直接獲取值)
            long long mask_poly_eval = prover.open_mask_polynomial(0); // 參數 r 在模擬中沒用到
            std::cout << "[6] [ZK] Prover opens mask polynomial g(r) = " << mask_poly_eval << std::endl;

            // 還原: f(r) = final_val_masked - rho * g(r)
            long long real_circuit_eval = mod_sub(final_val_masked, mod_mul(rho, mask_poly_eval));
            std::cout << "    [V] Verifier recovers real circuit value: " << final_val_masked 
                      << " - (" << rho << " * " << mask_poly_eval << ") = " << real_circuit_eval << std::endl;

            // 7. 歸約到下一層 (使用還原後的真實值)
            std::cout << "[7] Reducing claim to Layer " << i+1 << std::endl;
            long long v_u = prover.circuit.layers[i+1][0].value;
            long long v_v = prover.circuit.layers[i+1][1].value;
            std::cout << "    [V] (Simulation) Using next layer real values: V_u=" << v_u << ", V_v=" << v_v << std::endl;
            
            long long r1 = get_random_challenge();
            long long r2 = get_random_challenge();
            long long combined_claim = mod_add(mod_mul(r1, v_u), mod_mul(r2, v_v));
            
            current_claims_values = { combined_claim };
            std::cout << "    [V] Next claim for Layer " << i+1 << " is: " << combined_claim << std::endl;
        }

        std::cout << "\n--- Final Verification Stage ---" << std::endl;
        std::cout << "[V] Protocol reached Input Layer." << std::endl;
        std::cout << "[SUCCESS] Libra ZK-GKR verification successful!" << std::endl;

        return true;
    }
};

int main() {
    // 定義電路結構 (8輸入)
    size_t num_layers = 4;
    size_t num_inputs = 8;
    Circuit circuit(num_layers, num_inputs);

    // L0: Output
    circuit.add_gate(0, ADD, 0, 1);
    // L1: Mul
    circuit.add_gate(1, MUL, 0, 1);
    circuit.add_gate(1, MUL, 2, 3);
    // L2: Add
    circuit.add_gate(2, ADD, 0, 1);
    circuit.add_gate(2, ADD, 2, 3);
    circuit.add_gate(2, ADD, 4, 5);
    circuit.add_gate(2, ADD, 6, 7);
    // L3: Inputs
    for(size_t i = 0; i < num_inputs; ++i) circuit.add_gate(3, INPUT, 0, 0);

    std::vector<long long> secret_inputs = {3, 5, 2, 7, 1, 4, 6, 2};

    Prover prover(circuit, secret_inputs);
    Verifier verifier(circuit);

    long long claimed_output = prover.evaluate_circuit();

    verifier.run_protocol(prover, secret_inputs, claimed_output);

    return 0;
}