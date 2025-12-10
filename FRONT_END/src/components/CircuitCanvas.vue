<template>
  <div class="canvas" ref="container">
    <p style="padding:12px;color:var(--muted)">Circuit canvas placeholder</p>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
const container = ref(null)
onMounted(()=>{
  // placeholder for canvas initialization
})
</script>

<style scoped>
.canvas{ width:100%; box-sizing:border-box; }
</style>