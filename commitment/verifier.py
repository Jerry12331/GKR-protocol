import sys, os, time

COMMIT_FILE = "commitment.txt"
REVEAL_FILE = "reveal.txt"

G = 5
P = 997

def main():
    if not os.path.exists(COMMIT_FILE):
        print("[Verifier] 找不到 commitment.txt，請先讓 Prover 進行承諾")
        return

    with open(COMMIT_FILE, "r", encoding="utf-8") as f:
        C = int(f.readline().strip())
    
    print(f"[Verifier] 已讀取承諾 C = {C}")
    print("[Verifier] 等待揭露檔案 reveal.txt ... (按 Ctrl+C 結束)")
    
    try:
        while True:
            if os.path.exists(REVEAL_FILE):
                with open(REVEAL_FILE, "r", encoding="utf-8") as f:
                    lines = [l.strip() for l in f.readlines() if l.strip()]
                
                if len(lines) < 2:
                    print("[Verifier] reveal.txt 格式錯誤")
                    return
                
                m_str = lines[0]
                r = int(lines[1])
                
                m_int = int.from_bytes(m_str.encode('utf-8'), 'big')
                
                exponent = m_int + r
                C_check = pow(G, exponent, P)
                
                ok = (C_check == C)
                
                print(f"[Verifier] 揭露內容 m = {m_str}, r = {r}")
                print(f"[Verifier] 計算 ({G}^(m_int+r)) mod {P} = {C_check}")
                print("[Verifier] 驗證結果:", "通過" if ok else "失敗")
                return
            time.sleep(1)
    except KeyboardInterrupt:
        print("\n[Verifier] 中止")

if __name__ == "__main__":
    main()