<template>
  <div :class="['chat-bubble', { 'from-me': fromMe }]">
    <slot />
  </div>
</template>

<script setup>
import { defineProps } from 'vue'
const props = defineProps({ fromMe: { type: Boolean, default: false } })
</script>

<style scoped>
.chat-bubble{ max-width:70%; }
.chat-bubble.from-me{ background: #3b82f6; color: white; }
</style>