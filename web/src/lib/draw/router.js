// Tiny hash router. Routes are stored in `location.hash`, e.g. "#/draw".
//
//   import { route, goto } from './router.js';
//   $route === '/' | '/draw' | '/print'
//   goto('/draw')

import { writable } from 'svelte/store';

const parse = () => location.hash.replace(/^#/, '') || '/';

export const route = writable(parse());

window.addEventListener('hashchange', () => route.set(parse()));

export function goto(path) {
  if (location.hash !== '#' + path) location.hash = path;
}
