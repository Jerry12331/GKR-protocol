import sys, os, secrets, time

COMMIT_FILE = "commitment.txt"
REVEAL_FILE = "reveal.txt"
TEMP_DATA_FILE = ".last_data"

G = 5
P = 997

def main():
    if len(sys.argv) < 2:
        print("用法: python prover.py <message>    # 進行承諾")
        print("   或: python prover.py --reveal      # 揭露承諾")
        return

    if sys.argv[1] == "--reveal":
        if not os.path.exists(TEMP_DATA_FILE):
            print(f"[Prover] 找不到暫存檔案 {TEMP_DATA_FILE}，無法揭露")
            return
        
        with open(TEMP_DATA_FILE, "r", encoding="utf-8") as f:
            lines = [l.strip() for l in f.readlines()]
            message = lines[0]
            r = lines[1]
        
        with open(REVEAL_FILE, "w", encoding="utf-8") as f:
            f.write(f"{message}\n{r}\n{int(time.time())}\n")
        print(f"[Prover] 已寫入 {REVEAL_FILE} -> (m, r) 揭露完成")
        return

    message = sys.argv[1]
    m_int = int.from_bytes(message.encode('utf-8'), 'big')
    
    r = secrets.randbelow(P)
    
    exponent = m_int + r
    C = pow(G, exponent, P)

    with open(COMMIT_FILE, "w", encoding="utf-8") as f:
        f.write(str(C) + "\n")
    
    with open(TEMP_DATA_FILE, "w", encoding="utf-8") as f:
        f.write(f"{message}\n{r}\n")
    
    print(f"[Prover] 承諾完成。 C = {C}")
    print(f"[Prover] 秘密資料已儲存於 {TEMP_DATA_FILE} 以供後續揭露")

if __name__ == "__main__":
    main()