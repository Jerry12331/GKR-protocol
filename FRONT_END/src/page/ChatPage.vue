<template>
  <div class="chat-page-container">
    
    <div class="top-section">
      <div class="circuit-wrapper">
        <CircuitCanvas :activeLayer="currentLayer" />
        
        <div class="layer-indicator">
          <span :class="{ active: currentLayer === 0 }">L0 (Out)</span> → 
          <span :class="{ active: currentLayer === 1 }">L1</span> → 
          <span :class="{ active: currentLayer === 2 }">L2</span> → 
          <span :class="{ active: currentLayer === 3 }">L3 (In)</span>
        </div>
      </div>

      <div class="state-board card">
        <div class="board-header">
          <h4>📊 Math State</h4>
          <label class="god-mode-toggle">
            <input type="checkbox" v-model="godMode"> 👁️ God Mode
          </label>
        </div>
        <div class="board-content">
          <div class="stat-item">
            <span class="label">Current Claim ($H$):</span>
            <span class="value highlight">{{ currentMathState.claim || '-' }}</span>
          </div>
          <div class="stat-item">
            <span class="label">Mask Sum ($G$):</span>
            <span class="value" :class="{ secret: !godMode }">
              {{ godMode ? (currentMathState.mask || '-') : (currentMathState.mask ? '🙈 Secret' : '-') }}
            </span>
          </div>
          <div class="stat-item">
            <span class="label">Challenge ($\rho / r$):</span>
            <span class="value random">{{ currentMathState.challenge || '-' }}</span>
          </div>
        </div>
      </div>
    </div>

    <div class="chat-section" ref="chatContainer">
      <div class="chat-stream">
        <div 
          v-for="step in visibleSteps" 
          :key="step.id" 
          class="message-row"
          :class="step.speaker"
        >
          <div v-if="step.speaker === 'verifier'" class="avatar verifier-avatar">🛡️</div>
          
          <div class="bubble-container">
            <div class="speaker-name">{{ step.speaker === 'verifier' ? 'Verifier' : 'Prover' }}</div>
            <div class="bubble" :class="[step.type, step.speaker]">
              <div class="msg-content" v-html="formatMath(step.message)"></div>
              <div v-if="step.type === 'zk_mask'" class="zk-tag">🎭 ZK Masking</div>
            </div>
          </div>

          <div v-if="step.speaker === 'prover'" class="avatar prover-avatar">🎩</div>
        </div>
      </div>
    </div>

    <div class="controls-section">
      <div class="progress-bar">
        Step {{ currentStepIndex }} / {{ totalSteps }}
      </div>
      <div class="buttons">
        <button class="btn-ctrl" @click="prevStep" :disabled="currentStepIndex <= 0">⬅️ Prev</button>
        <button class="btn-ctrl play-btn" @click="toggleAutoPlay">
          {{ isAutoPlaying ? '⏸️ Pause' : '▶️ Auto Play' }}
        </button>
        <button class="btn-ctrl" @click="nextStep" :disabled="currentStepIndex >= totalSteps">Next ➡️</button>
      </div>
    </div>

  </div>
</template>

<script setup>
import { ref, computed, watch, nextTick, onMounted } from 'vue';
import CircuitCanvas from '../components/CircuitCanvas.vue';

// --- 狀態管理 ---
const currentStepIndex = ref(0);
const godMode = ref(false); // 上帝視角開關
const isAutoPlaying = ref(false);
const chatContainer = ref(null);
let autoPlayInterval = null;

// --- Mock Data (模擬後端回傳的完整驗證流程) ---
const protocolSteps = [
  { id: 1, speaker: 'prover', type: 'info', layer: 0, message: '我聲稱這個電路的輸出是 <b>15</b>', state: { claim: 15 } },
  { id: 2, speaker: 'verifier', type: 'challenge', layer: 0, message: '收到。開始驗證 Layer 0 -> Layer 1。請證明！', state: { claim: 15 } },
  
  // ZK Masking 階段
  { id: 3, speaker: 'prover', type: 'zk_mask', layer: 0, message: '發送遮罩總和 G (隱藏真實數值)', state: { claim: 15, mask: 88 } },
  { id: 4, speaker: 'verifier', type: 'challenge', layer: 0, message: '發送隨機挑戰參數 𝜌 = 5', state: { claim: 15, mask: 88, challenge: 5 } },
  { id: 5, speaker: 'verifier', type: 'info', layer: 0, message: '計算新目標: H + 𝜌G = 15 + 5*88 = <b>455</b>', state: { claim: 455, mask: 88, challenge: 5 } },
  
  // Sumcheck 階段
  { id: 6, speaker: 'prover', type: 'poly', layer: 0, message: '發送 Sumcheck 多項式 p(x) = 10 + 92x', state: { claim: 455 } },
  { id: 7, speaker: 'verifier', type: 'check', layer: 0, message: '檢查: p(0)+p(1) = 455? ✅ 通過', state: { claim: 455 } },
  { id: 8, speaker: 'verifier', type: 'challenge', layer: 0, message: '發送隨機挑戰 r = 80', state: { claim: 455, challenge: 80 } },
  
  // 歸約階段
  { id: 9, speaker: 'verifier', type: 'info', layer: 0, message: 'Sumcheck 結束。移除遮罩，還原 Layer 1 真實值...', state: { claim: 455 } },
  { id: 10, speaker: 'verifier', type: 'info', layer: 1, message: '歸約完成。Layer 1 的新目標值是 <b>24</b>', state: { claim: 24 } },
  
  // Layer 1
  { id: 11, speaker: 'verifier', type: 'challenge', layer: 1, message: '現在開始驗證 Layer 1 -> Layer 2', state: { claim: 24 } },
  { id: 12, speaker: 'prover', type: 'zk_mask', layer: 1, message: '發送 Layer 1 的遮罩 G', state: { claim: 24, mask: 123 } },
  { id: 13, speaker: 'verifier', type: 'info', layer: 1, message: '...(省略中間步驟)...', state: { claim: 24, mask: 123 } },
  
  // Final
  { id: 99, speaker: 'verifier', type: 'success', layer: 3, message: '🎉 驗證成功！所有檢查通過。', state: { claim: 'Pass' } }
];

// --- Computed ---
const totalSteps = protocolSteps.length;

const visibleSteps = computed(() => {
  return protocolSteps.slice(0, currentStepIndex.value);
});

const currentLayer = computed(() => {
  if (currentStepIndex.value === 0) return 0;
  return protocolSteps[currentStepIndex.value - 1].layer;
});

const currentMathState = computed(() => {
  if (currentStepIndex.value === 0) return {};
  // 總是顯示最新一步的狀態，如果最新一步沒有定義某個欄位，可以往回找(這裡簡化為只看當前)
  return protocolSteps[currentStepIndex.value - 1].state || {};
});

// --- Methods ---
const nextStep = () => {
  if (currentStepIndex.value < totalSteps) {
    currentStepIndex.value++;
    scrollToBottom();
  } else {
    stopAutoPlay(); // 到底停止
  }
};

const prevStep = () => {
  if (currentStepIndex.value > 0) {
    currentStepIndex.value--;
  }
};

const toggleAutoPlay = () => {
  if (isAutoPlaying.value) {
    stopAutoPlay();
  } else {
    startAutoPlay();
  }
};

const startAutoPlay = () => {
  isAutoPlaying.value = true;
  autoPlayInterval = setInterval(() => {
    nextStep();
  }, 1500); // 每 1.5 秒一步
};

const stopAutoPlay = () => {
  isAutoPlaying.value = false;
  clearInterval(autoPlayInterval);
};

const scrollToBottom = async () => {
  await nextTick();
  if (chatContainer.value) {
    chatContainer.value.scrollTop = chatContainer.value.scrollHeight;
  }
};

// 簡單的格式化 (可以替換成 KaTeX)
const formatMath = (text) => {
  return text.replace(/\n/g, '<br>');
};

// Keyboard support
onMounted(() => {
  window.addEventListener('keydown', (e) => {
    if (e.key === 'ArrowRight') nextStep();
    if (e.key === 'ArrowLeft') prevStep();
  });
});

</script>

<style scoped>
/* 版面配置 */
.chat-page-container {
  display: flex;
  flex-direction: column;
  height: calc(100vh - 60px);
  background-color: var(--bg);
  padding: 10px;
  gap: 10px;
  box-sizing: border-box;
}

/* 1. 上方區塊 */
.top-section {
  flex: 0 0 160px; /* 固定高度 */
  display: flex;
  gap: 15px;
}

.circuit-wrapper {
  flex: 2;
  background: white;
  border-radius: 8px;
  border: 1px solid #ddd;
  padding: 10px;
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: center;
}

.layer-indicator {
  margin-top: 10px;
  font-family: monospace;
  font-size: 14px;
  color: #999;
}
.layer-indicator span.active {
  color: #2196f3;
  font-weight: bold;
  font-size: 16px;
  text-decoration: underline;
}

.state-board {
  flex: 1;
  background: #2d3748;
  color: white;
  border-radius: 8px;
  padding: 12px;
  display: flex;
  flex-direction: column;
}

.board-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 10px;
  border-bottom: 1px solid #4a5568;
  padding-bottom: 5px;
}
.board-header h4 { margin: 0; }

.god-mode-toggle {
  font-size: 12px;
  cursor: pointer;
  user-select: none;
}

.stat-item {
  display: flex;
  justify-content: space-between;
  margin-bottom: 5px;
  font-size: 14px;
}
.stat-item .label { color: #a0aec0; }
.stat-item .value { font-family: monospace; font-weight: bold; }
.stat-item .value.highlight { color: #63b3ed; }
.stat-item .value.random { color: #d6bcfa; }
.stat-item .value.secret { color: #f56565; filter: blur(4px); transition: filter 0.3s; }
.stat-item .value.secret:hover { filter: blur(0); } /* 滑鼠移過去偷看 */

/* 2. 中間聊天區 */
.chat-section {
  flex: 1;
  background: #f1f5f9;
  border-radius: 8px;
  border: 1px solid #e2e8f0;
  overflow-y: auto;
  padding: 20px;
  display: flex;
  flex-direction: column;
}

.chat-stream {
  display: flex;
  flex-direction: column;
  gap: 15px;
  padding-bottom: 20px;
}

.message-row {
  display: flex;
  gap: 10px;
  max-width: 80%;
  align-items: flex-end;
}

/* Verifier (Left) */
.message-row.verifier {
  align-self: flex-start;
}
.verifier-avatar { font-size: 24px; }

/* Prover (Right) */
.message-row.prover {
  align-self: flex-end;
  flex-direction: row-reverse;
  text-align: right;
}
.prover-avatar { font-size: 24px; }

.bubble-container {
  display: flex;
  flex-direction: column;
}

.speaker-name {
  font-size: 12px;
  color: #64748b;
  margin-bottom: 2px;
}

.bubble {
  padding: 10px 15px;
  border-radius: 12px;
  position: relative;
  font-size: 15px;
  line-height: 1.4;
  box-shadow: 0 1px 2px rgba(0,0,0,0.1);
}

/* Bubble Styles */
.bubble.verifier {
  background: white;
  border-bottom-left-radius: 2px;
  border: 1px solid #e2e8f0;
  color: #1e293b;
}

.bubble.prover {
  background: #dcfce7; /* Light Green */
  border-bottom-right-radius: 2px;
  border: 1px solid #bbf7d0;
  color: #14532d;
}

.bubble.zk_mask {
  border: 2px solid #f87171; /* Red Border for ZK */
  background: #fef2f2;
}

.zk-tag {
  font-size: 10px;
  color: #ef4444;
  font-weight: bold;
  margin-top: 4px;
  text-transform: uppercase;
}

/* 3. 下方控制區 */
.controls-section {
  flex: 0 0 60px;
  background: white;
  border-top: 1px solid #e2e8f0;
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 0 20px;
  border-radius: 8px;
}

.progress-bar {
  color: #64748b;
  font-family: monospace;
}

.buttons {
  display: flex;
  gap: 10px;
}

.btn-ctrl {
  padding: 8px 16px;
  border: 1px solid #cbd5e1;
  background: white;
  border-radius: 6px;
  cursor: pointer;
  transition: all 0.2s;
  font-weight: 500;
}
.btn-ctrl:hover:not(:disabled) { background: #f1f5f9; transform: translateY(-1px); }
.btn-ctrl:disabled { opacity: 0.5; cursor: not-allowed; }

.play-btn {
  background: #3b82f6;
  color: white;
  border-color: #2563eb;
  width: 120px;
}
.play-btn:hover:not(:disabled) { background: #2563eb; }

</style>