const { execSync } = require('child_process');
const fs = require('fs');
const path = require('path');

console.log('🔄 Building CSS with Tailwind...');

// Build do CSS
execSync('npx tailwindcss -i ./src/styles/input.css -o ./public/styles.css --minify', { 
  stdio: 'inherit'
});

// Copiar arquivos HTML
console.log('📁 Copying HTML files...');
if (!fs.existsSync('./public')) {
  fs.mkdirSync('./public');
}

const files = fs.readdirSync('./src');
files.forEach(file => {
  if (path.extname(file) === '.html') {
    fs.copyFileSync(`./src/${file}`, `./public/${file}`);
    console.log(`✅ Copied: ${file}`);
  }
});

console.log('🎉 Build completed!');
