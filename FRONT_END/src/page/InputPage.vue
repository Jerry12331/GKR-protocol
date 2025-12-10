<template>
  <div class="container">
    <h2>Input Page</h2>
    <form @submit.prevent="send">
      <input v-model="text" placeholder="Type message" style="padding:8px;width:60%" />
      <button class="button" type="submit" style="margin-left:8px">Send</button>
    </form>
  </div>
</template>

<script setup>
import { ref } from 'vue'
const text = ref('')
function send(){
  console.log('send', text.value)
  text.value = ''
}
</script>

<style scoped>
.container{ max-width:900px; margin:16px auto }
</style>