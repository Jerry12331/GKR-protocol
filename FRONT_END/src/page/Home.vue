<template>
  <div class="home-container">
    
    <div class="sidebar-config">
      <div class="sidebar-header">
        <h2>🛠️ 設計電路</h2>
        <div class="input-setting">
          <label>輸入變數 (Input Size)</label>
          <input type="number" v-model.number="inputSize" min="1" @change="adjustInputLayer" />
        </div>
      </div>

      <div class="layers-scroll-area">
        <div 
          v-for="(layer, layerIdx) in layers" 
          :key="layerIdx" 
          class="layer-card"
          :class="{ 
            'output-layer': layerIdx === 0, 
            'input-layer': layerIdx === layers.length - 1,
            'active-verifying': currentLayer === layerIdx && isProtocolStarted 
          }"
        >
          <div class="layer-header">
            <h4>
              <span v-if="layerIdx === 0">🏁 Output (L0)</span>
              <span v-else-if="layerIdx === layers.length - 1">🚀 Input (L{{ layerIdx }})</span>
              <span v-else>⛓️ Middle (L{{ layerIdx }})</span>
            </h4>
            <div class="layer-tools" v-if="layerIdx < layers.length - 1">
              <button class="btn-xs add" @click="addGate(layerIdx)">+ 加閘</button>
              <button v-if="layerIdx > 0" class="btn-xs del" @click="removeLayer(layerIdx)">刪除層</button>
            </div>
          </div>

          <div class="gates-list" v-if="layerIdx < layers.length - 1">
            <div v-for="(gate, gateIdx) in layer.gates" :key="gateIdx" class="gate-row">
              <span class="gate-badge">G{{ gateIdx }}</span>
              <select v-model="gate.type" class="type-select">
                <option value="ADD">+</option>
                <option value="MUL">×</option>
              </select>
              <div class="conn-inputs">
                <span>⬇ L{{layerIdx+1}}:</span>
                <input type="number" v-model.number="gate.in1" class="idx-input" placeholder="0">
                <input type="number" v-model.number="gate.in2" class="idx-input" placeholder="1">
              </div>
              <button class="btn-icon" @click="removeGate(layerIdx, gateIdx)">×</button>
            </div>
            <div v-if="layer.gates.length === 0" class="empty-hint">此層無閘</div>
          </div>

          <div class="gates-list" v-else>
            <div class="input-config-box">
              <div class="mode-select">
                <label>輸入模式：</label>
                <select v-model="inputMode" @change="applyInputMode">
                  <option value="manual">🖐 手動輸入 (Manual)</option>
                  <option value="constant">🔁 全部相同 (Constant)</option>
                  <option value="arithmetic">📈 等差數列 (Progression)</option>
                  <option value="random">🎲 全部隨機 (Random)</option>
                </select>
              </div>

              <div v-if="inputMode === 'manual'" class="inputs-grid">
                <div v-for="(val, idx) in secretInputs" :key="idx" class="input-cell">
                  <label>x{{ idx }}</label>
                  <input type="number" v-model.number="secretInputs[idx]">
                </div>
              </div>

              <div v-else-if="inputMode === 'constant'" class="struct-form">
                <div class="form-group">
                  <label>數值 (Value):</label>
                  <input type="number" v-model.number="structParams.constVal" @input="applyInputMode">
                </div>
                <small class="hint">所有 {{ inputSize }} 個輸入都將設為 {{ structParams.constVal }}</small>
              </div>

              <div v-else-if="inputMode === 'arithmetic'" class="struct-form">
                <div class="form-row">
                  <div class="form-group">
                    <label>起始:</label>
                    <input type="number" v-model.number="structParams.start" @input="applyInputMode">
                  </div>
                  <div class="form-group">
                    <label>公差:</label>
                    <input type="number" v-model.number="structParams.step" @input="applyInputMode">
                  </div>
                </div>
                <small class="hint">預覽: {{ secretInputs.slice(0,3).join(', ') }} ...</small>
              </div>

              <div v-else-if="inputMode === 'random'" class="struct-form">
                <button class="btn-dashed" @click="applyInputMode">🎲 重新隨機生成</button>
                <small class="hint">已生成 {{ inputSize }} 個隨機數</small>
              </div>
            </div>
          </div>
          
        </div>

        <button class="btn-dashed" @click="addNewLayer">+ 插入中間層</button>
      </div>

      <div class="sidebar-footer">
        <button class="btn-run" @click="startVerification" :disabled="isProtocolStarted">
          {{ isProtocolStarted ? '驗證進行中...' : '🚀 鎖定並開始驗證' }}
        </button>
        <button v-if="isProtocolStarted" class="btn-reset" @click="resetAll">🔄 重置</button>
      </div>
    </div>

    <div class="main-content">
      
      <div class="viz-section">
        <div class="viz-header">
          <h3>👁️ 電路視覺化</h3>
          <div class="legend">
            <span class="dot out"></span> Output
            <span class="dot in"></span> Input
            <span class="dot active"></span> Verifying
          </div>
        </div>
        <div class="canvas-wrapper">
          <CircuitCanvas 
            :circuitData="serializedData" 
            :activeLayer="isProtocolStarted ? currentLayer : -1" 
          />
        </div>
      </div>

      <div class="chat-section" v-if="isProtocolStarted">
        <div class="chat-header">
          <h3>💬 GKR 互動驗證</h3>
          <div class="math-status">
            <div class="status-item">
              <small>Current Claim (H)</small>
              <strong>{{ currentMathState.claim || '-' }}</strong>
            </div>
            <div class="status-item">
              <small>Mask Sum (G)</small>
              <strong :class="{ blur: !godMode }">{{ currentMathState.mask || '-' }}</strong>
            </div>
            <div class="status-item">
              <small>Challenge (r)</small>
              <strong class="purple">{{ currentMathState.challenge || '-' }}</strong>
            </div>
            <label class="god-mode-check">
              <input type="checkbox" v-model="godMode"> God Mode
            </label>
          </div>
        </div>

        <div class="messages-area" ref="chatRef">
          <div 
            v-for="step in visibleSteps" 
            :key="step.id" 
            class="msg-row" 
            :class="step.speaker"
          >
            <div class="avatar">{{ step.speaker === 'verifier' ? '🛡️' : '🎩' }}</div>
            <div class="bubble" :class="[step.type, step.speaker]">
              <div class="bubble-name">{{ step.speaker.toUpperCase() }}</div>
              <div v-html="step.message"></div>
              <div v-if="step.type === 'zk_mask'" class="zk-badge">🎭 ZK MASK</div>
            </div>
          </div>
        </div>

        <div class="chat-controls">
          <div class="step-info">Step {{ currentStepIndex }} / {{ totalSteps }}</div>
          <div class="btn-group">
            <button @click="prevStep" :disabled="currentStepIndex<=0">⬅ Prev</button>
            <button @click="toggleAuto" class="btn-play">
              {{ isAutoPlaying ? '⏸ Pause' : '▶ Auto Play' }}
            </button>
            <button @click="nextStep" :disabled="currentStepIndex>=totalSteps">Next ➡</button>
          </div>
        </div>
      </div>

      <div class="empty-chat-placeholder" v-else>
        <div class="placeholder-content">
          <span class="icon">👈</span>
          <h3>請先在左側設計電路</h3>
          <p>完成後點擊「鎖定並開始驗證」來啟動 GKR 協議</p>
        </div>
      </div>

    </div>
  </div>
</template>

<script setup>
import { ref, computed, nextTick } from 'vue';
import CircuitCanvas from '../components/CircuitCanvas.vue';

// === 電路設計邏輯 ===
const inputSize = ref(8);
const secretInputs = ref([3, 5, 2, 7, 1, 4, 6, 2]);
const layers = ref([
  { gates: [{ type: 'ADD', in1: 0, in2: 1 }] },
  { gates: [{ type: 'MUL', in1: 0, in2: 1 }, { type: 'MUL', in1: 2, in2: 3 }] },
  { gates: [] } // 輸入層
]);

// === 結構化輸入邏輯 ===
const inputMode = ref('manual');
const structParams = ref({ constVal: 1, start: 1, step: 1 });

const applyInputMode = () => {
  // 確保陣列長度正確
  if (secretInputs.value.length !== inputSize.value) {
    secretInputs.value = new Array(inputSize.value).fill(0);
  }
  if (inputMode.value === 'constant') {
    secretInputs.value.fill(structParams.value.constVal);
  } else if (inputMode.value === 'arithmetic') {
    for (let i = 0; i < inputSize.value; i++) {
      secretInputs.value[i] = structParams.value.start + (i * structParams.value.step);
    }
  } else if (inputMode.value === 'random') {
    for (let i = 0; i < inputSize.value; i++) {
      secretInputs.value[i] = Math.floor(Math.random() * 100);
    }
  }
};

const adjustInputLayer = () => {
  const targetLen = inputSize.value;
  if (targetLen > secretInputs.value.length) {
    const diff = targetLen - secretInputs.value.length;
    for(let i=0; i<diff; i++) secretInputs.value.push(0);
  } else {
    secretInputs.value = secretInputs.value.slice(0, targetLen);
  }
  // 若非手動模式，自動重填
  if (inputMode.value !== 'manual') applyInputMode();
};

const addGate = (idx) => layers.value[idx].gates.push({ type: 'ADD', in1:0, in2:0 });
const removeGate = (lIdx, gIdx) => layers.value[lIdx].gates.splice(gIdx, 1);
const addNewLayer = () => layers.value.splice(layers.value.length-1, 0, { gates: [{type:'ADD',in1:0,in2:0}] });
const removeLayer = (idx) => layers.value.splice(idx, 1);

// 序列化資料
const serializedData = computed(() => ({
  layers: layers.value.map((l, i) => 
    i === layers.value.length - 1 
      ? Array.from({length: inputSize.value}, () => ({ type: 'INPUT' }))
      : l.gates
  ),
  num_inputs: inputSize.value
}));

// === 驗證邏輯 ===
const isProtocolStarted = ref(false);
const currentStepIndex = ref(0);
const godMode = ref(false);
const isAutoPlaying = ref(false);
const chatRef = ref(null);
let autoTimer = null;

// Mock Data (完整流程)
const protocolSteps = [
  { id:1, speaker:'prover', type:'info', layer:0, message:'我聲稱輸出結果是 <b>15</b>', state:{claim:15} },
  { id:2, speaker:'verifier', type:'challenge', layer:0, message:'收到。開始驗證 Layer 0 (Output)。請證明！', state:{claim:15} },
  { id:3, speaker:'prover', type:'zk_mask', layer:0, message:'[ZK] 發送遮罩 G0 (隱藏真實值)', state:{claim:15, mask:88} },
  { id:4, speaker:'verifier', type:'challenge', layer:0, message:'隨機挑戰 ρ0 = 5', state:{claim:15, mask:88, challenge:5} },
  { id:5, speaker:'verifier', type:'info', layer:0, message:'新目標 H + ρG = 455', state:{claim:455, mask:88, challenge:5} },
  { id:6, speaker:'prover', type:'poly', layer:0, message:'Sumcheck 多項式: 10 + 92x', state:{claim:455} },
  { id:7, speaker:'verifier', type:'success', layer:0, message:'Sumcheck 通過！', state:{claim:455} },
  { id:8, speaker:'verifier', type:'challenge', layer:0, message:'隨機選擇歸約點 r = 80', state:{claim:455, challenge:80} },
  { id:9, speaker:'verifier', type:'info', layer:1, message:'歸約至 Layer 1，目標值 <b>24</b>', state:{claim:24} },
  { id:10, speaker:'prover', type:'zk_mask', layer:1, message:'[ZK] Layer 1 遮罩 G1', state:{claim:24, mask:123} },
  { id:11, speaker:'verifier', type:'challenge', layer:1, message:'隨機挑戰 ρ1 = 12', state:{claim:24, mask:123, challenge:12} },
  { id:12, speaker:'verifier', type:'info', layer:1, message:'新目標 1500', state:{claim:1500, mask:123, challenge:12} },
  { id:13, speaker:'prover', type:'poly', layer:1, message:'Sumcheck 多項式: 20 + 81x', state:{claim:1500} },
  { id:14, speaker:'verifier', type:'success', layer:1, message:'Sumcheck 通過！', state:{claim:1500} },
  { id:15, speaker:'verifier', type:'info', layer:2, message:'歸約至 Layer 2，目標值 <b>55</b>', state:{claim:55} },
  { id:16, speaker:'prover', type:'zk_mask', layer:2, message:'[ZK] Layer 2 遮罩 G2', state:{claim:55, mask:42} },
  { id:17, speaker:'verifier', type:'challenge', layer:2, message:'隨機挑戰 ρ2 = 7', state:{claim:55, mask:42, challenge:7} },
  { id:18, speaker:'verifier', type:'info', layer:2, message:'新目標 349', state:{claim:349, mask:42, challenge:7} },
  { id:19, speaker:'prover', type:'poly', layer:2, message:'Sumcheck 多項式: 30 + 92x', state:{claim:349} },
  { id:20, speaker:'verifier', type:'success', layer:2, message:'Sumcheck 通過！', state:{claim:349} },
  { id:21, speaker:'verifier', type:'info', layer:3, message:'抵達輸入層。檢查公開輸入...', state:{claim:'Final'} },
  { id:99, speaker:'verifier', type:'success', layer:3, message:'🎉 驗證成功！Prover 是誠實的。', state:{claim:'Pass'} }
];

const totalSteps = protocolSteps.length;
const visibleSteps = computed(() => protocolSteps.slice(0, currentStepIndex.value));
const currentLayer = computed(() => currentStepIndex.value === 0 ? 0 : (protocolSteps[currentStepIndex.value - 1]?.layer || 0));
const currentMathState = computed(() => currentStepIndex.value === 0 ? {} : (protocolSteps[currentStepIndex.value - 1]?.state || {}));

const startVerification = () => { isProtocolStarted.value = true; currentStepIndex.value = 1; };
const resetAll = () => { isProtocolStarted.value = false; currentStepIndex.value = 0; isAutoPlaying.value = false; clearInterval(autoTimer); };
const nextStep = () => { if (currentStepIndex.value < totalSteps) { currentStepIndex.value++; nextTick(() => { if (chatRef.value) chatRef.value.scrollTop = chatRef.value.scrollHeight; }); } else stopAuto(); };
const prevStep = () => { if(currentStepIndex.value > 0) currentStepIndex.value--; };
const toggleAuto = () => isAutoPlaying.value ? stopAuto() : startAuto();
const startAuto = () => { isAutoPlaying.value = true; autoTimer = setInterval(nextStep, 1500); };
const stopAuto = () => { isAutoPlaying.value = false; clearInterval(autoTimer); };

adjustInputLayer();
</script>

<style scoped>
.home-container { display: flex; height: 100vh; overflow: hidden; background: #f0f2f5; font-family: 'Segoe UI', sans-serif; }
.sidebar-config { width: 340px; background: white; border-right: 1px solid #ddd; display: flex; flex-direction: column; z-index: 10; }
.sidebar-header { padding: 20px; border-bottom: 1px solid #eee; background: #f8f9fa; }
.sidebar-header h2 { margin: 0 0 10px 0; font-size: 20px; }
.layers-scroll-area { flex: 1; overflow-y: auto; padding: 15px; background: #fafafa; }
.layer-card { background: white; border: 1px solid #e0e0e0; border-radius: 8px; margin-bottom: 15px; overflow: hidden; }
.layer-card.output-layer { border-left: 4px solid #f56565; }
.layer-card.input-layer { border-left: 4px solid #48bb78; }
.layer-card.active-verifying { border-color: #4299e1; box-shadow: 0 0 0 2px rgba(66, 153, 225, 0.3); transform: scale(1.02); }
.layer-header { padding: 10px; background: #fff; border-bottom: 1px solid #f0f0f0; display: flex; justify-content: space-between; align-items: center; }
.layer-header h4 { margin: 0; font-size: 14px; color: #555; }
.gates-list { padding: 10px; }
.gate-row { display: flex; align-items: center; gap: 5px; margin-bottom: 8px; background: #f7fafc; padding: 6px; border-radius: 4px; }
.gate-badge { font-size: 11px; font-weight: bold; color: #718096; width: 25px; }
.type-select { width: 50px; }
.conn-inputs { display: flex; align-items: center; gap: 3px; font-size: 11px; color: #718096; }
.idx-input { width: 30px; padding: 2px; text-align: center; border: 1px solid #cbd5e1; border-radius: 3px; }
.inputs-grid { display: grid; grid-template-columns: repeat(4, 1fr); gap: 5px; }
.input-cell input { width: 100%; text-align: center; }
.btn-xs { padding: 2px 6px; font-size: 11px; border-radius: 4px; border: 1px solid #ccc; cursor: pointer; }
.btn-icon { background: none; border: none; color: #cbd5e1; cursor: pointer; margin-left: auto; }
.btn-dashed { width: 100%; border: 1px dashed #cbd5e1; background: none; padding: 8px; color: #718096; cursor: pointer; border-radius: 6px; }
.sidebar-footer { padding: 15px; border-top: 1px solid #ddd; background: white; display: flex; gap: 10px; }
.btn-run { flex: 1; background: #3182ce; color: white; border: none; padding: 12px; border-radius: 6px; cursor: pointer; font-weight: bold; }
.btn-run:disabled { background: #cbd5e1; cursor: not-allowed; }
.btn-reset { background: #718096; color: white; border: none; padding: 12px; border-radius: 6px; cursor: pointer; }

/* 結構化輸入表單樣式 */
.input-config-box { background: #f8fafc; padding: 10px; border-radius: 6px; border: 1px dashed #cbd5e1; }
.mode-select { margin-bottom: 10px; display: flex; flex-direction: column; gap: 5px; font-size: 12px; color: #64748b; }
.mode-select select { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; background: white; }
.struct-form { display: flex; flex-direction: column; gap: 8px; }
.form-group { display: flex; align-items: center; gap: 8px; font-size: 12px; }
.form-group input { flex: 1; padding: 4px; border: 1px solid #cbd5e1; border-radius: 3px; }
.form-row { display: flex; gap: 10px; }
.hint { font-size: 10px; color: #94a3b8; font-style: italic; }
.empty-hint { text-align: center; color: #ccc; font-size: 12px; padding: 10px; }

/* 右側樣式 */
.main-content { flex: 1; display: flex; flex-direction: column; height: 100vh; }
.viz-section { flex: 5; background: #e2e8f0; display: flex; flex-direction: column; border-bottom: 2px solid #cbd5e1; position: relative; }
.viz-header { padding: 10px 20px; background: rgba(255,255,255,0.8); backdrop-filter: blur(4px); position: absolute; top: 0; left: 0; right: 0; z-index: 5; display: flex; justify-content: space-between; align-items: center; }
.viz-header h3 { margin: 0; font-size: 16px; }
.legend { font-size: 12px; color: #718096; display: flex; gap: 10px; align-items: center; }
.dot { width: 8px; height: 8px; border-radius: 50%; display: inline-block; margin-right: 4px; }
.dot.out { background: #f56565; }
.dot.in { background: #48bb78; }
.dot.active { background: #4299e1; box-shadow: 0 0 4px #4299e1; }
.canvas-wrapper { width: 100%; height: 100%; }

.chat-section { flex: 5; background: white; display: flex; flex-direction: column; overflow: hidden; }
.chat-header { padding: 10px 20px; border-bottom: 1px solid #edf2f7; display: flex; justify-content: space-between; align-items: center; background: #fff; }
.math-status { display: flex; gap: 15px; align-items: center; background: #f7fafc; padding: 5px 15px; border-radius: 20px; border: 1px solid #edf2f7; }
.status-item { display: flex; flex-direction: column; align-items: center; line-height: 1.2; }
.status-item small { font-size: 10px; color: #a0aec0; }
.status-item strong { font-family: monospace; font-size: 14px; }
.status-item strong.purple { color: #805ad5; }
.status-item strong.blur { color: transparent; text-shadow: 0 0 5px rgba(0,0,0,0.5); }
.messages-area { flex: 1; overflow-y: auto; padding: 20px; background: #f8fafc; display: flex; flex-direction: column; gap: 12px; }
.msg-row { display: flex; gap: 10px; max-width: 80%; }
.msg-row.verifier { align-self: flex-start; }
.msg-row.prover { align-self: flex-end; flex-direction: row-reverse; }
.avatar { font-size: 24px; }
.bubble { padding: 10px 14px; border-radius: 12px; font-size: 14px; line-height: 1.5; position: relative; box-shadow: 0 1px 2px rgba(0,0,0,0.05); }
.bubble-name { font-size: 10px; font-weight: bold; margin-bottom: 2px; opacity: 0.7; }
.verifier .bubble { background: white; border: 1px solid #e2e8f0; border-bottom-left-radius: 2px; }
.prover .bubble { background: #f0fff4; border: 1px solid #c6f6d5; border-bottom-right-radius: 2px; }
.bubble.zk_mask { border: 2px solid #fc8181; background: #fff5f5; }
.zk-badge { display: inline-block; background: #fc8181; color: white; font-size: 9px; padding: 1px 4px; border-radius: 3px; margin-top: 4px; }
.chat-controls { padding: 10px 20px; border-top: 1px solid #edf2f7; display: flex; justify-content: space-between; align-items: center; }
.btn-group button { padding: 6px 12px; margin-left: 5px; border: 1px solid #cbd5e1; background: white; border-radius: 4px; cursor: pointer; }
.btn-group .btn-play { background: #ebf8ff; color: #3182ce; border-color: #bee3f8; }
.empty-chat-placeholder { flex: 5; display: flex; justify-content: center; align-items: center; color: #a0aec0; background: #fcfcfc; }
.placeholder-content { text-align: center; }
.placeholder-content .icon { font-size: 40px; display: block; margin-bottom: 10px; animation: bounce 2s infinite; }
@keyframes bounce { 0%, 100% { transform: translateX(0); } 50% { transform: translateX(-10px); } }
</style>