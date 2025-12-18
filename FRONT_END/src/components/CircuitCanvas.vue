<template>
  <div class="circuit-board" ref="boardRef">
    <svg width="100%" height="100%">
      <g class="wires">
        <path 
          v-for="(wire, idx) in wires" 
          :key="`w-${idx}`"
          :d="getWirePath(wire)"
          stroke="#cbd5e1"
          stroke-width="2"
          fill="none"
          class="wire-path"
        />
      </g>

      <g class="layers">
        <g v-for="(layer, lIdx) in vizLayers" :key="`l-${lIdx}`">
          <text 
            :x="layer.x" 
            y="30" 
            text-anchor="middle" 
            class="layer-label"
            :class="{ active: activeLayer === lIdx }"
          >
            {{ getLayerName(lIdx, vizLayers.length) }}
          </text>

          <g 
            v-for="(gate, gIdx) in layer.gates" 
            :key="`g-${lIdx}-${gIdx}`"
            :transform="`translate(${gate.x}, ${gate.y})`"
            class="gate-group"
          >
            <circle 
              r="18" 
              class="gate-circle" 
              :class="{ 
                'is-input': lIdx === vizLayers.length - 1,
                'is-output': lIdx === 0,
                'active': activeLayer === lIdx
              }"
            />
            <text dy="5" text-anchor="middle" class="gate-text">
              {{ getGateLabel(gate) }}
            </text>
            <text dy="-22" text-anchor="middle" class="gate-id">
              #{{ gIdx }}
            </text>
          </g>
        </g>
      </g>
    </svg>
  </div>
</template>

<script setup>
import { computed, defineProps } from 'vue';

const props = defineProps({
  // 電路資料結構
  circuitData: {
    type: Object,
    default: () => ({ layers: [], num_inputs: 0 }) 
  },
  // 當前驗證層級 (用來高亮)
  activeLayer: {
    type: Number,
    default: -1
  }
});

// --- 視覺化參數 ---
const CANVAS_PADDING = 60;
const LAYER_SPACING = 150; 
const GATE_SPACING = 60;   

// --- 計算座標 ---
const vizLayers = computed(() => {
  const rawLayers = props.circuitData.layers || [];
  if (rawLayers.length === 0) return [];

  // 找最大層以垂直置中
  const maxGates = Math.max(...rawLayers.map(l => Array.isArray(l) ? l.length : (l.gates ? l.gates.length : 0)));
  const maxHeight = maxGates * GATE_SPACING;

  return rawLayers.map((layerData, lIdx) => {
    // 統一資料格式 (InputPage 傳來的是物件陣列, Home 傳來的是 gates 陣列)
    let gates = Array.isArray(layerData) ? layerData : (layerData.gates || []);

    // 如果是 Input Layer 且是空的，補齊
    if (lIdx === rawLayers.length - 1 && gates.length === 0 && props.circuitData.num_inputs) {
      gates = Array.from({ length: props.circuitData.num_inputs }, () => ({ type: 'INPUT' }));
    }

    const layerHeight = gates.length * GATE_SPACING;
    const startY = (maxHeight - layerHeight) / 2 + CANVAS_PADDING;

    return {
      idx: lIdx,
      x: CANVAS_PADDING + lIdx * LAYER_SPACING,
      gates: gates.map((g, gIdx) => ({
        ...g,
        x: CANVAS_PADDING + lIdx * LAYER_SPACING,
        y: startY + gIdx * GATE_SPACING
      }))
    };
  });
});

// --- 計算連線 ---
const wires = computed(() => {
  const layers = vizLayers.value;
  const links = [];

  for (let i = 0; i < layers.length - 1; i++) {
    const currentLayer = layers[i];
    const nextLayer = layers[i + 1];

    currentLayer.gates.forEach((gate) => {
      // 檢查 in1
      if (gate.in1 !== undefined && gate.in1 !== null && nextLayer.gates[gate.in1]) {
        links.push({ 
          x1: gate.x, y1: gate.y, 
          x2: nextLayer.gates[gate.in1].x, y2: nextLayer.gates[gate.in1].y 
        });
      }
      // 檢查 in2
      if (gate.in2 !== undefined && gate.in2 !== null && nextLayer.gates[gate.in2]) {
        links.push({ 
          x1: gate.x, y1: gate.y, 
          x2: nextLayer.gates[gate.in2].x, y2: nextLayer.gates[gate.in2].y 
        });
      }
    });
  }
  return links;
});

const getWirePath = (w) => {
  const midX = (w.x1 + w.x2) / 2;
  return `M ${w.x1} ${w.y1} C ${midX} ${w.y1}, ${midX} ${w.y2}, ${w.x2} ${w.y2}`;
};

const getLayerName = (idx, total) => {
  if (idx === 0) return "Output (L0)";
  if (idx === total - 1) return `Input (L${idx})`;
  return `L${idx}`;
};

const getGateLabel = (gate) => {
  if (gate.type === 'ADD') return '+';
  if (gate.type === 'MUL') return '×';
  return 'In';
};
</script>

<style scoped>
.circuit-board {
  width: 100%;
  height: 100%;
  min-height: 400px;
  background-color: #f8fafc;
  border-radius: 8px;
  overflow: auto;
}
.layer-label { font-size: 12px; fill: #64748b; font-weight: bold; }
.layer-label.active { fill: #2563eb; }
.gate-circle { fill: white; stroke: #94a3b8; stroke-width: 2; transition: all 0.3s; }
.gate-circle.is-output { stroke: #ef4444; }
.gate-circle.is-input { stroke: #22c55e; }
.gate-circle.active { stroke: #3b82f6; stroke-width: 3; filter: drop-shadow(0 0 4px rgba(59, 130, 246, 0.5)); }
.gate-text { font-size: 18px; font-weight: bold; fill: #334155; pointer-events: none; }
.gate-id { font-size: 10px; fill: #94a3b8; }
.wire-path { transition: stroke 0.3s; }
</style>