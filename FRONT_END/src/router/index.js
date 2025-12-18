import { createRouter, createWebHistory } from 'vue-router'
import Home from '../page/Home.vue' // [重要] 引入 Home

const routes = [
  { path: '/', name: 'home', component: Home }, // [重要] 讓首頁直接顯示 Home
  // 其他舊的路由可以留著備用
  // { path: '/input', ... }
]

const router = createRouter({ history: createWebHistory(), routes })
export default router