<template>
  <StickyHeader>
    <div style="display:flex;align-items:center;gap:12px">
      <h3 style="margin:0">My App</h3>
      <nav>
        <a href="/">Chat</a> | <a href="/input">Input</a>
      </nav>
    </div>
  </StickyHeader>
  <router-view />
</template>

<script setup>
import StickyHeader from './components/StickyHeader.vue'
</script>

<style>
:root{ --bg: #f8fafc }
</style>