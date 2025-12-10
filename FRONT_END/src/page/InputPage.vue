<template>
  <div class="input-page-container">
    <div class="config-panel">
      <h2>🛠️ 電路設計 (Circuit Builder)</h2>
      
      <div class="global-settings card">
        <h3>全域設定</h3>
        <div class="form-row">
          <label>輸入變數數量 (Input Size):</label>
          <input type="number" v-model.number="inputSize" min="1" @change="adjustInputLayer" />
        </div>
      </div>

      <div class="layers-container">
        <div 
          v-for="(layer, layerIdx) in layers" 
          :key="layerIdx" 
          class="layer-card"
          :class="{ 'output-layer': layerIdx === 0, 'input-layer': layerIdx === layers.length - 1 }"
        >
          <div class="layer-header">
            <h4>
              <span v-if="layerIdx === 0">🏁 Output Layer (Layer 0)</span>
              <span v-else-if="layerIdx === layers.length - 1">🚀 Input Layer (Layer {{ layerIdx }})</span>
              <span v-else>⛓️ Layer {{ layerIdx }} (Middle)</span>
            </h4>
            
            <div class="layer-controls" v-if="layerIdx < layers.length - 1">
              <button class="btn-small add-gate" @click="addGate(layerIdx)">+ 加閘 (Gate)</button>
            </div>
            
            <button 
              v-if="layerIdx > 0 && layerIdx < layers.length - 1" 
              class="btn-small delete-layer"
              @click="removeLayer(layerIdx)"
            >
              刪除層
            </button>
          </div>

          <div class="gates-list">
            <template v-if="layerIdx < layers.length - 1">
              <div v-for="(gate, gateIdx) in layer.gates" :key="gateIdx" class="gate-item">
                <span class="gate-id">G{{ gateIdx }}</span>
                
                <select v-model="gate.type">
                  <option value="ADD">ADD (+)</option>
                  <option value="MUL">MUL (×)</option>
                </select>

                <div class="connections">
                  <span class="arrow">⬇️ 連接下一層 (L{{ layerIdx + 1 }}) 的:</span>
                  <input type="number" v-model.number="gate.in1" placeholder="Idx 1" min="0" class="input-idx">
                  <input type="number" v-model.number="gate.in2" placeholder="Idx 2" min="0" class="input-idx">
                </div>

                <button class="btn-icon remove-gate" @click="removeGate(layerIdx, gateIdx)">✕</button>
              </div>
              <div v-if="layer.gates.length === 0" class="empty-hint">
                此層尚無閘，請點擊「+ 加閘」
              </div>
            </template>

            <template v-else>
              <div class="inputs-grid">
                <div v-for="(val, idx) in secretInputs" :key="idx" class="input-box">
                  <label>Input {{ idx }}</label>
                  <input type="number" v-model.number="secretInputs[idx]" />
                </div>
              </div>
            </template>
          </div>
        </div>
      </div>

      <div class="action-bar">
        <button class="btn-secondary" @click="addNewLayer">⬇️ 插入中間層</button>
        <button class="btn-primary" @click="submitCircuit">🚀 送出並開始驗證 (Test Submit)</button>
      </div>
    </div>

    <div class="preview-panel">
      <div class="sticky-wrapper">
        <h3>📄 序列化資訊 (Serialized Info)</h3>
        <p class="desc">這就是即將傳送給後端 C++/Python 的資料結構。</p>
        <pre class="json-box">{{ JSON.stringify(serializedData, null, 2) }}</pre>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, watch } from 'vue';
import { useRouter } from 'vue-router';

const router = useRouter();

// --- 狀態定義 ---

// 預設輸入數量
const inputSize = ref(8);

// 輸入層的具體數值 (Secret Inputs)
const secretInputs = ref([3, 5, 2, 7, 1, 4, 6, 2]);

// 電路結構：預設包含一個 Output Layer 和一個 Input Layer
// 每個 Layer 是一個物件 { gates: [] }
// 閘物件結構: { type: 'ADD', in1: 0, in2: 0 }
const layers = ref([
  { // Layer 0 (Output)
    gates: [
      { type: 'ADD', in1: 0, in2: 1 } // 預設連到 L1 的 Gate 0 和 1
    ]
  },
  { // Layer 1 (Example Middle)
    gates: [
      { type: 'MUL', in1: 0, in2: 1 },
      { type: 'MUL', in2: 2, in2: 3 }
    ]
  },
  { // Layer 2 (Last layer is always treated as Input Layer conceptually)
    gates: [] // Input layer doesn't really define gates logic here, mostly handled by inputSize
  }
]);

// --- 方法 ---

// 調整輸入層的數值陣列大小
const adjustInputLayer = () => {
  const currentLen = secretInputs.value.length;
  const targetLen = inputSize.value;

  if (targetLen > currentLen) {
    for (let i = 0; i < targetLen - currentLen; i++) {
      secretInputs.value.push(0);
    }
  } else if (targetLen < currentLen) {
    secretInputs.value = secretInputs.value.slice(0, targetLen);
  }
};

// 新增閘
const addGate = (layerIdx) => {
  layers.value[layerIdx].gates.push({ type: 'ADD', in1: 0, in2: 0 });
};

// 移除閘
const removeGate = (layerIdx, gateIdx) => {
  layers.value[layerIdx].gates.splice(gateIdx, 1);
};

// 插入新層 (插在倒數第二層位置，保持 Input Layer 永遠在最後)
const addNewLayer = () => {
  const insertIdx = layers.value.length - 1;
  const newLayer = {
    gates: [{ type: 'ADD', in1: 0, in2: 0 }]
  };
  layers.value.splice(insertIdx, 0, newLayer);
};

// 移除層
const removeLayer = (layerIdx) => {
  layers.value.splice(layerIdx, 1);
};

// --- 計算屬性 (序列化) ---
const serializedData = computed(() => {
  // 將 Vue 的響應式資料轉換為後端需要的格式
  // 注意：我們需要處理最後一層(Input Layer)，在 C++ 邏輯中它包含 INPUT 類型的 Gate
  
  const formattedLayers = layers.value.map((layer, idx) => {
    // 如果是最後一層，根據 inputSize 生成 INPUT gates
    if (idx === layers.value.length - 1) {
      return Array.from({ length: inputSize.value }, () => ({
        type: 'INPUT',
        in1: 0,
        in2: 0
      }));
    }
    // 其他層直接回傳 gates
    return layer.gates.map(g => ({
      type: g.type,
      in1: g.in1,
      in2: g.in2
    }));
  });

  return {
    num_layers: layers.value.length,
    num_inputs: inputSize.value,
    layers: formattedLayers,
    secret_inputs: secretInputs.value
  };
});

// --- 送出 ---
const submitCircuit = () => {
  console.log("🚀 Submitting Circuit Data:", serializedData.value);
  alert("資料已打包送出！請查看 Console (F12)。\n接下來可以跳轉到 ChatPage 開始互動。");
  
  // 這裡之後可以串接後端 API
  // fetch('http://localhost:5000/api/init_circuit', { ... })
  
  // 或者直接跳轉 (模擬)
  // router.push('/chat');
};

// 初始化
adjustInputLayer();

</script>

<style scoped>
/* 佈局容器 */
.input-page-container {
  display: flex;
  gap: 20px;
  padding: 20px;
  height: calc(100vh - 60px); /* 扣掉 Header 高度 */
  box-sizing: border-box;
}

/* 左側面板 */
.config-panel {
  flex: 2;
  overflow-y: auto;
  padding-right: 10px;
}

/* 右側面板 */
.preview-panel {
  flex: 1;
  background-color: #1e1e1e; /* 深色背景適合看 code */
  color: #d4d4d4;
  border-radius: 8px;
  padding: 15px;
  overflow: hidden;
  display: flex;
  flex-direction: column;
}

.sticky-wrapper {
  position: sticky;
  top: 0;
  height: 100%;
  display: flex;
  flex-direction: column;
}

.json-box {
  background: #000;
  padding: 10px;
  border-radius: 4px;
  overflow: auto;
  flex-grow: 1;
  font-family: 'Courier New', Courier, monospace;
  font-size: 14px;
  color: #ce9178; /* JSON string color mimic */
}

/* 通用卡片樣式 */
.card, .layer-card {
  background: white;
  border: 1px solid #ddd;
  border-radius: 8px;
  padding: 15px;
  margin-bottom: 20px;
  box-shadow: 0 2px 4px rgba(0,0,0,0.05);
}

.layer-card {
  border-left: 5px solid #ccc;
}
.layer-card.output-layer { border-left-color: #ff5252; } /* 紅色輸出層 */
.layer-card.input-layer { border-left-color: #4caf50; } /* 綠色輸入層 */

/* Layer Header */
.layer-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 10px;
  border-bottom: 1px solid #eee;
  padding-bottom: 5px;
}

.layer-header h4 {
  margin: 0;
  color: #333;
}

/* Gates */
.gate-item {
  display: flex;
  align-items: center;
  gap: 10px;
  background: #f9f9f9;
  padding: 8px;
  border-radius: 4px;
  margin-bottom: 5px;
  border: 1px solid #eee;
}

.gate-id {
  font-weight: bold;
  color: #666;
  width: 30px;
}

.connections {
  display: flex;
  align-items: center;
  gap: 5px;
  font-size: 12px;
  color: #666;
}

.input-idx {
  width: 60px;
  padding: 4px;
}

/* Inputs Grid */
.inputs-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(80px, 1fr));
  gap: 10px;
}
.input-box {
  display: flex;
  flex-direction: column;
  font-size: 12px;
}
.input-box input {
  width: 100%;
  padding: 5px;
  box-sizing: border-box;
}

/* Buttons */
.btn-small {
  padding: 4px 8px;
  font-size: 12px;
  cursor: pointer;
  border: 1px solid #ccc;
  background: #fff;
  border-radius: 4px;
}
.add-gate { color: #2196f3; border-color: #2196f3; }
.delete-layer { color: #f44336; border-color: #f44336; }
.remove-gate {
  border: none;
  background: none;
  color: #999;
  cursor: pointer;
  margin-left: auto; /* Push to right */
}
.remove-gate:hover { color: red; }

.action-bar {
  display: flex;
  gap: 15px;
  margin-top: 20px;
}

.btn-primary {
  flex: 2;
  padding: 12px;
  background-color: #2196f3;
  color: white;
  border: none;
  border-radius: 6px;
  font-size: 16px;
  cursor: pointer;
  transition: background 0.2s;
}
.btn-primary:hover { background-color: #1976d2; }

.btn-secondary {
  flex: 1;
  padding: 12px;
  background-color: #607d8b;
  color: white;
  border: none;
  border-radius: 6px;
  cursor: pointer;
}
.btn-secondary:hover { background-color: #455a64; }

.form-row {
  display: flex;
  align-items: center;
  gap: 10px;
}
</style>