import { readFileSync } from 'fs'
import { resolve, dirname } from 'path'
import { fileURLToPath } from 'url'
import { defineConfig } from 'vite'

const __dirname = dirname(fileURLToPath(import.meta.url))

function readVersion() {
  try {
    const versionRaw = readFileSync(resolve(__dirname, '../VERSION'), 'utf-8').trim()
    const version = versionRaw.replace(/^\/+\s*/, '')
    return version || 'dev'
  } catch (e) {
    console.warn('VERSION file not found, using "dev"')
    return 'dev'
  }
}

export default defineConfig({
  define: {
    __APP_VERSION__: JSON.stringify(readVersion())
  },
  root: '.',
  build: {
    outDir: 'dist',
    emptyOutDir: true,
    minify: 'terser',
    terserOptions: {
      compress: {
        drop_console: true,
        drop_debugger: true
      }
    },
    rollupOptions: {
      output: {
        manualChunks: undefined,
        assetFileNames: 'assets/[name][extname]',
        chunkFileNames: 'assets/[name].js',
        entryFileNames: 'assets/[name].js'
      }
    }
  },
  server: {
    port: 8000,
    host: '127.0.0.1'
  },
  preview: {
    port: 8000,
    host: '127.0.0.1'
  }
})
