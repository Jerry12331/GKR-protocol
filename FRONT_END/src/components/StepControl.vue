<template>
  <div class="step-control">
    <button class="button" @click="$emit('prev')">Prev</button>
    <button class="button" style="margin-left:8px" @click="$emit('next')">Next</button>
  </div>
</template>

<script setup>
// emits: prev, next
</script>

<style scoped>
.step-control{ display:flex; align-items:center }
.button{ padding:6px 10px }
</style>