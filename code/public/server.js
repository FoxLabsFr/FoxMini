const express = require('express');
const path = require('path');
const app = express();
const port = 3000;

// Serve static files from the current directory
app.use(express.static(__dirname));

// Serve the main page
app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, 'index.html'));
});

// Start the server
app.listen(port, '0.0.0.0', () => {
  console.log(`Robot control server running at:`);
  console.log(`- Local: http://localhost:${port}`);
  console.log(`- Network: http://0.0.0.0:${port}`);
  console.log(`- Phone: http://[YOUR_COMPUTER_IP]:${port}`);
  console.log('');
  console.log('To find your computer IP address:');
  console.log('- Windows: ipconfig');
  console.log('- Mac/Linux: ifconfig or ip addr');
  console.log('');
  console.log('Make sure your phone and computer are on the same WiFi network!');
});

// Handle 404 errors
app.use((req, res) => {
  res.status(404).send('Page not found');
}); 