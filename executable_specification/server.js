const express = require('express');
const multer = require('multer');
const path = require('path');
const fs = require('fs');
const { spawn } = require('child_process');

const app = express();

// Set up multer with a custom storage configuration to save as 'input_file.txt'
const uploadsDir = path.join(__dirname, 'src', 'uploads');

const storage = multer.diskStorage({
  destination: (req, file, cb) => {
    cb(null, uploadsDir); 
  },
  filename: (req, file, cb) => {
    cb(null, 'input_file.txt');
  }
});


const upload = multer({ storage: storage });

const PORT = process.env.PORT || 3000;
const baseAddress = path.resolve(__dirname); // Base path

// Paths for different components
const INPUT_FILE_PATH = `${baseAddress}/src/uploads/input_file.txt`;
const OUTPUT_FILE_PATH = `${baseAddress}/src/output_file.txt`;
const cppSource = `${baseAddress}/src`; // Your C++ source code
const outputDir = `${baseAddress}/bin`; // Directory for compiled binaries

// Serve static files (e.g., HTML, CSS, JS)
app.use(express.static(path.join(__dirname, 'public')));

app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, 'public', 'user_interface.html'));
});

// Parse JSON bodies for requests
app.use(express.json());

// Endpoint to handle file upload
app.post('/upload', upload.single('inputFile'), (req, res) => {
  if (!req.file) {
    return res.status(400).send('No file uploaded.');
  }

  const binaryPath = path.join(outputDir, 'specification_dijkstra');

  console.log(`Running C++ binary: ${binaryPath} ${INPUT_FILE_PATH} ${OUTPUT_FILE_PATH}`);

  // Run the compiled C++ binary using `spawn`
  const process = spawn(binaryPath, [INPUT_FILE_PATH, OUTPUT_FILE_PATH]);

  process.stdout.on('data', (data) => {
    console.log(`${data}`);
  });

  process.stderr.on('data', (data) => {
    console.error(`stderr: ${data}`);
  });

  process.on('close', (code) => {
    console.log(`Process exited with code ${code}`);

    if (code !== 0) {
      return res.status(500).send(`C++ process exited with code ${code}`);
    }

    // Read and send output file content
    fs.readFile(OUTPUT_FILE_PATH, 'utf8', (err, data) => {
      if (err) {
        console.error('Error reading output file:', err);
        return res.status(500).send('Error reading output file.');
      }

      const route = parseRouteData(data);
      res.json({ route });
    });
  });
});

// Endpoint to run the already compiled C++ program
app.get('/run-cpp', (req, res) => {
  const binaryPath = path.join(outputDir, 'specification_dijkstra'); // Path to the compiled binary

  console.log(`Running the compiled binary: ${binaryPath}`);

  // Ensure the binary exists and is executable
  fs.access(binaryPath, fs.constants.F_OK | fs.constants.X_OK, (err) => {
    if (err) {
      console.error(`Binary not found or not executable: ${binaryPath}`);
      return res.status(500).send(`Error: Binary not found or not executable at ${binaryPath}`);
    }

    // Run the compiled binary
    const runProcess = spawn(binaryPath, [INPUT_FILE_PATH, OUTPUT_FILE_PATH]);

    runProcess.stdout.on('data', (data) => {
      console.log(`Execution stdout: ${data}`);
    });

    runProcess.stderr.on('data', (data) => {
      console.error(`Execution stderr: ${data}`);
    });

    runProcess.on('close', (code) => {
      if (code !== 0) {
        return res.status(500).send(`Execution failed with exit code ${code}`);
      }

      // Read and send output file content
      fs.readFile(OUTPUT_FILE_PATH, 'utf8', (err, data) => {
        if (err) {
          console.error('Error reading output file:', err);
          return res.status(500).send('Error reading output file.');
        }

        const route = parseRouteData(data);
        res.json({ route });
      });
    });
  });
});

// Function to parse the output file (assume it contains lat, lng pairs)
function parseRouteData(data) {
  const route = [];
  const lines = data.split('\n');

  lines.forEach(line => {
    const [lat, lng] = line.trim().split(' ');
    if (lat && lng) {
      route.push({ lat: parseFloat(lat), lng: parseFloat(lng) });
    }
  });

  return route;
}

// Function to clear input_file.txt and output_file.txt
function clearFiles() {
  const inputFilePath = INPUT_FILE_PATH;
  const outputFilePath = OUTPUT_FILE_PATH;

  fs.writeFile(inputFilePath, '', (err) => {
    if (err) {
      console.error('Error clearing input_file.txt:', err);
    } else {
      console.log('Cleared input_file.txt');
    }
  });

  fs.writeFile(outputFilePath, '', (err) => {
    if (err) {
      console.error('Error clearing output_file.txt:', err);
    } else {
      console.log('Cleared output_file.txt');
    }
  });
}

// Start the server
app.listen(PORT, () => {
  console.log(`Server running on port ${PORT}`);
  const openUrl = `http://localhost:${PORT}`;
  spawn('xdg-open', [openUrl]); // Open browser automatically (Linux)
});
