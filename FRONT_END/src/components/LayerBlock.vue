<template>
  <div class="layer-block">
    <header>{{ title }}</header>
    <div class="content">
      <slot />
    </div>
  </div>
</template>

<script setup>
import { defineProps } from 'vue'
const props = defineProps({ title: { type: String, default: 'Layer' } })
</script>

<style scoped>
.layer-block{ border:1px solid #e5e7eb; padding:8px; border-radius:6px; background:white }
.layer-block header{ font-weight:600; margin-bottom:6px }
</style>