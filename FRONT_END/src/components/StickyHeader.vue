<template>
  <div class="header" style="position:sticky;top:0;z-index:10">
    <slot />
  </div>
</template>

<script setup>
</script>

<style scoped>
.header{ display:flex; align-items:center; justify-content:space-between }
</style>