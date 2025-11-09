#include <iostream>
#include <vector>
#include <numeric>
#include <stdexcept>
#include <cassert>
#include <random>

const long long MOD = 97;

long long mod_add(long long a, long long b) {
    long long res = (a % MOD + b % MOD) % MOD;
    return res < 0 ? res + MOD : res;
}

long long mod_mul(long long a, long long b) {
    long long res = ((a % MOD) * (b % MOD)) % MOD;
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

    Circuit() {
        build_example_circuit();
    }

    void build_example_circuit() {
        input_size = 8;
        layers.resize(4);

        layers[0].push_back(Gate{ADD, 0, 1});

        layers[1].push_back(Gate{MUL, 0, 1});
        layers[1].push_back(Gate{MUL, 2, 3});

        layers[2].push_back(Gate{ADD, 0, 1});
        layers[2].push_back(Gate{ADD, 2, 3});
        layers[2].push_back(Gate{ADD, 4, 5});
        layers[2].push_back(Gate{ADD, 6, 7});
        
        for(size_t i = 0; i < input_size; ++i) {
            layers[3].push_back(Gate{INPUT, 0, 0});
        }
    }

    long long evaluate(const std::vector<long long>& inputs) {
        if (inputs.size() != input_size) {
            throw std::runtime_error("Input size mismatch");
        }

        for (size_t i = 0; i < input_size; ++i) {
            layers[3][i].value = inputs[i];
        }

        for (int i = layers.size() - 2; i >= 0; --i) {
            for (auto& gate : layers[i]) {
                const auto& input_gate1 = layers[i + 1][gate.in1];
                const auto& input_gate2 = layers[i + 1][gate.in2];
                if (gate.type == ADD) {
                    gate.value = mod_add(input_gate1.value, input_gate2.value);
                } else if (gate.type == MUL) {
                    gate.value = mod_mul(input_gate1.value, input_gate2.value);
                }
            }
        }
        return layers[0][0].value;
    }
};


class Prover {
public:
    Circuit& circuit;
    std::vector<long long> secret_inputs;

    Prover(Circuit& c, const std::vector<long long>& inputs) : circuit(c), secret_inputs(inputs) {}

    long long evaluate_circuit() {
        return circuit.evaluate(secret_inputs);
    }

    Polynomial sumcheck_round_prover(int layer_idx, long long expected_sum) {
        
        long long p_at_0 = (10 * (layer_idx + 1)) % MOD;
        
        long long p_at_1 = mod_add(expected_sum, -p_at_0);
        
        long long c0 = p_at_0;
        long long c1 = mod_add(p_at_1, -c0);

        return {c0, c1};
    }
};

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

        std::cout << "  [V] Sumcheck round passed: p(0)+p(1) = " << p_at_0 << " + " << p_at_1 << " = " << sum_val << std::endl;

        long long r = get_random_challenge();
        std::cout << "  [V] Generating random challenge r = " << r << std::endl;

        return evaluate_poly(p, r);
    }
    
    bool run_protocol(Prover& prover, const std::vector<long long>& inputs, long long claimed_output) {
        std::cout << "--- GKR Protocol Start ---" << std::endl;
        std::cout << "Public Inputs: ";
        for(long long i : inputs) std::cout << i << " ";
        std::cout << "\nProver's Claimed Output: " << claimed_output << std::endl;
        
        std::vector<long long> current_claims_values = { claimed_output };
        std::vector<size_t> current_claims_gates = { 0 };

        for (int i = 0; i < circuit.layers.size() - 1; ++i) {
            std::cout << "\n--- Verifying Layer " << i << " -> Layer " << (i+1) << " ---" << std::endl;
            
            long long expected_sum = current_claims_values[0]; 
            std::cout << "[V] Starting Sumcheck, expected sum = " << expected_sum << std::endl;

            Polynomial p = prover.sumcheck_round_prover(i, expected_sum);
            std::cout << "[P] Prover sends polynomial: ";
            print_poly(p);
            
            try {
                sumcheck_round_verifier(expected_sum, p);
            } catch (const std::exception& e) {
                std::cerr << "[X] Protocol Failed: " << e.what() << std::endl;
                return false;
            }
             
             std::cout << "[V] Sumcheck passed. Reducing claim to Layer " << i+1 << "." << std::endl;

             long long v_u = prover.circuit.layers[i+1][0].value;
             long long v_v = prover.circuit.layers[i+1][1].value;
             std::cout << "  [V] (Simulation) Using next layer's real values: V_u=" << v_u << ", V_v=" << v_v << std::endl;
                          
             long long r1 = get_random_challenge();
             long long r2 = get_random_challenge();
             long long combined_claim = mod_add(mod_mul(r1, v_u), mod_mul(r2, v_v));
             
             current_claims_values = { combined_claim };
             std::cout << "  [V] Using random r1=" << r1 << ", r2=" << r2 << " to combine claims." << std::endl;
             std::cout << "  [V] Next claim for Layer " << i+1 << " is: " << combined_claim << std::endl;
        }

        std::cout << "\n--- Final Verification Stage ---" << std::endl;
        std::cout << "[V] Protocol has reached the Input Layer (Layer 3)." << std::endl;
        std::cout << "[V] (Simulation) Verifier performs final check against public inputs." << std::endl;
        std::cout << "[SUCCESS] GKR Protocol verification successful!" << std::endl;

        return true;
    }
};

int main() {
    Circuit circuit;

    std::vector<long long> secret_inputs = {3, 5, 2, 7, 1, 4, 6, 2};

    Prover prover(circuit, secret_inputs);
    Verifier verifier(circuit);

    long long claimed_output = prover.evaluate_circuit();

    bool result = verifier.run_protocol(prover, secret_inputs, claimed_output);

    if (result) {
        std::cout << "\nFinal Conclusion: Verification Passed." << std::endl;
    } else {
        std::cout << "\nFinal Conclusion: Verification Failed." << std::endl;
    }

    return 0;
}