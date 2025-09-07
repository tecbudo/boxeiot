/** @type {import('tailwindcss').Config} */
module.exports = {
  content: ["./src/**/*.html"],
  theme: {
    extend: {
      colors: {
        'boxeiot-primary': '#1a2a6c',
        'boxeiot-secondary': '#b21f1f', 
        'boxeiot-accent': '#fdbb2d',
      },
      backgroundImage: {
        'boxeiot-gradient': 'linear-gradient(135deg, #1a2a6c, #b21f1f, #fdbb2d)',
      }
    },
  },
  plugins: [],
}
