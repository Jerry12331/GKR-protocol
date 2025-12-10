<template>
  <div class="container">
    <h2>Chat Page</h2>
    <div style="margin-bottom:12px">
      <ChatBubble>Hi there — this is a chat bubble.</ChatBubble>
      <ChatBubble fromMe>Reply from me</ChatBubble>
    </div>
    <CircuitCanvas />
  </div>
</template>

<script setup>
import ChatBubble from '../components/ChatBubble.vue'
import CircuitCanvas from '../components/CircuitCanvas.vue'
</script>

<style scoped>
.container{ max-width:900px; margin:16px auto }
</style>