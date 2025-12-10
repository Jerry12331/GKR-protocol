import { createRouter, createWebHistory } from 'vue-router'
import ChatPage from '../page/ChatPage.vue'
import InputPage from '../page/InputPage.vue'

const routes = [
  { path: '/', name: 'chat', component: ChatPage },
  { path: '/input', name: 'input', component: InputPage }
]

const router = createRouter({ history: createWebHistory(), routes })
export default router
