import { defineConfig } from 'vite'
import tailwindcss from '@tailwindcss/vite'

export default defineConfig({
  plugins: [
    tailwindcss(),
  ],
  build: {
    outDir: 'public', // Isso fará com que o build seja enviado para a pasta public
    emptyOutDir: true, // Limpa a pasta public antes de cada build
  }
})
