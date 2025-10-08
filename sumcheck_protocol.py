import random
import itertools
from numpy.polynomial import Polynomial


def target_polynomial(x1, x2, x3, x4):

    return 2 * x1 + 3 * x2 * x3 + x4

class Prover:
    def __init__(self, poly_func, num_vars):
        self.poly_func = poly_func
        self.num_vars = num_vars
        print(f"Prover: 多項式有 {num_vars} 個變數。")

    def calculate_total_sum(self):
        total_sum = 0
        for inputs in itertools.product([0, 1], repeat=self.num_vars):
            total_sum += self.poly_func(*inputs)
        print(f"Prover: 計算出所有 2^{self.num_vars} 種組合的總和 H = {total_sum}")
        return total_sum

    def generate_round_polynomial(self, round_index, challenges):      
        points = []
        num_points_needed = 3
        
        for x_val in range(num_points_needed):
            current_sum = 0
            fixed_vars = list(challenges) + [x_val]
            num_free_vars = self.num_vars - len(fixed_vars)
            for inputs in itertools.product([0, 1], repeat=num_free_vars):
                all_vars = tuple(fixed_vars) + inputs
                current_sum += self.poly_func(*all_vars)
            points.append(current_sum)

        x_coords = list(range(num_points_needed))
        y_coords = points
        
        round_poly = Polynomial.fit(x_coords, y_coords, deg=num_points_needed-1).convert()
        
        coeffs = [int(round(c)) for c in round_poly.coef]
        return Polynomial(coeffs)


class Verifier:
    def __init__(self, poly_func, num_vars):
        self.poly_func = poly_func
        self.num_vars = num_vars
        self.challenges = []
        print(f"Verifier: 準備驗證 {num_vars} 個變數的多項式。")

    def generate_random_challenge(self):
        return random.randint(2, 100)

    def run_protocol(self, prover):
        print("\n--- Sumcheck 開始 ---\n")
        
        claimed_sum_H = prover.calculate_total_sum()
        self.expected_sum_for_round = claimed_sum_H
        
        for i in range(self.num_vars):
            print(f"\n--- 第 {i+1} 回合 (處理變數 x_{i+1}) ---")
            
            round_poly = prover.generate_round_polynomial(i, self.challenges)
            print(f"Prover -> Verifier: 這是本回合的多項式 g_{i+1}(X) = {round_poly}")
            
            check_val = round_poly(0) + round_poly(1)
            print(f"Verifier: 正在檢查 g_{i+1}(0) + g_{i+1}(1) = {round_poly(0)} + {round_poly(1)} = {check_val}")
            print(f"Verifier: 上一回合的期望值為 {self.expected_sum_for_round}")
            
            if int(round(check_val)) != self.expected_sum_for_round:
                print("\n驗證失敗 Prover 在說謊")
                return False
            
            print(f"Verifier: 檢查通過")
            
            challenge = self.generate_random_challenge()
            self.challenges.append(challenge)
            print(f"Verifier -> Prover: 這是我的隨機挑戰 r_{i+1} = {challenge}")
            
            self.expected_sum_for_round = int(round(round_poly(challenge)))
            print(f"Verifier: 我計算出下一回合的期望值為 g_{i+1}({challenge}) = {self.expected_sum_for_round}")

        print("\n--- 所有回合結束，進行最終驗證 ---")
        final_prover_claim = self.expected_sum_for_round
        
        final_verifier_eval = self.poly_func(*self.challenges)
        
        print(f"Verifier: Prover 聲稱的最終結果是 {final_prover_claim}")
        print(f"Verifier: 我自己用挑戰值 {tuple(self.challenges)} 計算的結果是 {final_verifier_eval}")

        if final_prover_claim == final_verifier_eval:
            print("\n驗證成功 Prover 的原始總和聲稱是可信的")
            return True
        else:
            print("\n驗證失敗Prover 在最終步驟出錯")
            return False

if __name__ == "__main__":
    NUM_VARIABLES = 4

    prover = Prover(target_polynomial, NUM_VARIABLES)
    verifier = Verifier(target_polynomial, NUM_VARIABLES)

    verifier.run_protocol(prover)