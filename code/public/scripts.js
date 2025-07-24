// Configuration
const ESP_IP = "192.168.1.xxx"; // ESP8266 IP address
const API_BASE = `http://${ESP_IP}/api`;

// Make API_BASE globally available
window.API_BASE = API_BASE;



// Simple script loader
function fetchJson(url, onSuccess) {
  const fetchPromise = fetch(url)
    .then(response => {
      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }
      const contentType = response.headers.get('content-type');
      if (contentType && contentType.includes('application/json')) {
        return response.json();
      } else {
        // If not JSON, return the text as a simple object
        return response.text().then(text => ({ message: text }));
      }
    })
    .catch(error => {
      // Show loader on connection error
      const sectionsContainer = document.getElementById('sections-container');
      const loader = document.getElementById('loader');
      
      if (sectionsContainer) sectionsContainer.style.display = 'none';
      if (loader) {
        loader.classList.add('visible');
        loader.querySelector('.loader-text').innerText = 'Connection lost. Trying to reconnect...';
      }
      
      throw error;
    });
  
  if (onSuccess) {
    // Callback mode
    fetchPromise.then(onSuccess).catch(console.error);
  } else {
    // Promise mode
    return fetchPromise;
  }
}

// Make fetchJson globally available
window.fetchJson = fetchJson;

function getRobot() {
  fetchJson(`${API_BASE}?action=getRobot`, function(data) {
    // Show sections on successful connection
    const sectionsContainer = document.getElementById('sections-container');
    const loader = document.getElementById('loader');
    
    if (sectionsContainer && sectionsContainer.style.display === 'none') {
      sectionsContainer.style.display = 'flex';
      if (loader) loader.classList.remove('visible');
    }
    
    // Dispatch custom event with robot data
    const event = new CustomEvent('robotDataUpdate', { detail: data });
    document.dispatchEvent(event);
  });
}

// ===== LEGS SECTION =====
// Make updateServo function globally available
window.updateServo = function(arm, servo, angle) {
  fetchJson(`${API_BASE}?action=setPositions&arm=${arm}&servo=${servo}&angle=${angle}`, function() {
    document.getElementById(arm + 'GoalServoValue' + servo).innerText = angle;
  });
}

function updateServoValues(prefix, data) {
  // Handle new servo group format with type, state, and servos array
  let servosArray = data;
  if (data && data.type === 'servogroup' && data.servos) {
    servosArray = data.servos;
    
    // Update servo group type if element exists
    const typeElement = document.getElementById(prefix + 'Type');
    if (typeElement) {
      typeElement.innerText = data.type;
    }
    
    // Update servo group state if element exists
    const stateElement = document.getElementById(prefix + 'State');
    if (stateElement) {
      stateElement.innerText = data.state;
      // Update state color based on status
      if (data.state === 'moving') {
        stateElement.style.color = '#ff6b35'; // Orange for moving
      } else if (data.state === 'idle') {
        stateElement.style.color = '#28a745'; // Green for idle
      } else {
        stateElement.style.color = '#007bff'; // Blue for other states
      }
    }
  }
  
  // Handle legacy format (direct array) or new format (servos array)
  if (Array.isArray(servosArray)) {
    for (let i = 0; i < servosArray.length; ++i) {
      
      const currentServo = document.getElementById(prefix + 'CurrentServo' + i);
      const currentServoValue = document.getElementById(prefix + 'CurrentServoValue' + i);
      const goalServo = document.getElementById(prefix + 'GoalServo' + i);
      const goalServoValue = document.getElementById(prefix + 'GoalServoValue' + i);
      
      if (currentServo) {
        currentServo.value = servosArray[i].current;
      }
      if (currentServoValue) {
        currentServoValue.innerText = servosArray[i].current;
      }
      if (goalServo) {
        goalServo.value = servosArray[i].goal;
      }
      if (goalServoValue) {
        goalServoValue.innerText = servosArray[i].goal;
      }
      
      // Visual feedback: highlight servo values when they're moving
      const isMoving = Math.abs(servosArray[i].current - servosArray[i].goal) > 5;
      if (currentServoValue) {
        currentServoValue.style.color = isMoving ? '#ff6b35' : '#333';
        currentServoValue.style.fontWeight = isMoving ? 'bold' : 'normal';
        if (isMoving) {
          currentServoValue.classList.add('servo-moving');
        } else {
          currentServoValue.classList.remove('servo-moving');
        }
      }
      if (goalServoValue) {
        goalServoValue.style.color = isMoving ? '#ff6b35' : '#333';
        goalServoValue.style.fontWeight = isMoving ? 'bold' : 'normal';
        if (isMoving) {
          goalServoValue.classList.add('servo-moving');
        } else {
          goalServoValue.classList.remove('servo-moving');
        }
      }
    }
  }
}

// ===== JOYSTICK SECTION =====
function sendJoystickData(x, y) {
  fetchJson(`${API_BASE}?action=joystick&x=${x}&y=${y}`)
    .then(data => {
    })
    .catch(error => {
      console.error('sendJoystickData error:', error);
    });
}

// Walk control variables
let lastWalkRequest = 0;
let pendingWalkRequest = false;
let lastWalkValues = { x: 0, y: 0 };

function controlRobotWalk(x, y) {
  // Normalize x and y to -1.0 to 1.0 range
  let xDirection = 0;
  let yDirection = 0;
  
  // Calculate distance from center
  const distance = Math.sqrt(x * x + y * y);
  
  if (distance > 10) { // Only move if joystick is moved enough
    // Normalize the direction vectors
    xDirection = x / 100; // Convert from -100..100 to -1.0..1.0
    yDirection = -y / 100; // Invert Y axis: up = forward (positive), down = backward (negative)
  }
  

  
  // Update last walk values
  lastWalkValues.x = xDirection;
  lastWalkValues.y = yDirection;
  
  // Use throttled walk control
  throttledControlWalk();
}

// Throttled walk control function
function throttledControlWalk() {
  const now = Date.now();
  
  // If we're within the throttle window, schedule a request
  if (now - lastWalkRequest < 100) {
    if (!pendingWalkRequest) {
      pendingWalkRequest = true;
      setTimeout(() => {
        pendingWalkRequest = false;
        sendWalkControl();
      }, 100 - (now - lastWalkRequest));
    }
    return;
  }
  
  // Otherwise, make the request immediately
  lastWalkRequest = now;
  sendWalkControl();
}

function sendWalkControl() {
  const xDirection = lastWalkValues.x;
  const yDirection = lastWalkValues.y;
  

  
  // Update walking direction display if element exists
  const walkingDirectionElement = document.getElementById('walkingDirection');
  if (walkingDirectionElement) {
    const distance = Math.sqrt(xDirection * xDirection + yDirection * yDirection);
    if (distance < 0.1) {
      walkingDirectionElement.innerText = 'Stopped';
    } else {
      const direction = [];
      if (Math.abs(yDirection) > 0.1) {
        direction.push(yDirection > 0 ? 'Forward' : 'Backward');
      }
      if (Math.abs(xDirection) > 0.1) {
        direction.push(xDirection > 0 ? 'Right' : 'Left');
      }
      walkingDirectionElement.innerText = direction.length > 0 ? direction.join(' + ') : 'Stopped';
    }
  }
  
  // Get walking parameters from UI sliders
  const params = window.walkingParams || {};
  const stepLength = params.stepLength || 70;
  const stepHeight = params.stepHeight || 10;
  const stepDuration = params.stepDuration || 400;
  const walkHeight = params.walkHeight || 65;
  const xOffset = params.xOffset || 0.0;
  
  // Build URL with all parameters
  const url = `${API_BASE}?action=walkControl&x=${xDirection}&y=${yDirection}&stepLength=${stepLength}&stepHeight=${stepHeight}&stepDuration=${stepDuration}&walkHeight=${walkHeight}&xOffset=${xOffset}`;
  
  // Send walking command to robot with all parameters
  fetchJson(url)
    .then(data => {
    })
    .catch(error => {
      console.error('walkControl error:', error);
    });
}

// Make joystick function globally available
window.sendJoystickData = sendJoystickData;
window.controlRobotWalk = controlRobotWalk;



function toggleDriftCompensation() {
  const button = document.getElementById('driftToggle');
  const isEnabled = button.innerText.includes('Disable');
  const enable = !isEnabled;
  
  fetchJson(`${API_BASE}?action=toggleDriftCompensation&enable=${enable}`)
    .then(data => {
      if (enable) {
        button.innerText = 'Disable Drift Compensation';
        button.style.backgroundColor = '#4CAF50';
      } else {
        button.innerText = 'Enable Drift Compensation';
        button.style.backgroundColor = '#f44336';
      }
    })
    .catch(error => {
      console.error('toggleDriftCompensation error:', error);
    });
}

function resetDriftCompensation() {
  fetchJson(`${API_BASE}?action=resetDriftCompensation`)
    .then(data => {
    })
    .catch(error => {
      console.error('resetDriftCompensation error:', error);
    });
}

function startWalking() {
  const status = document.getElementById('walkingStatus');
  status.innerText = 'Starting walking...';
  status.style.color = '#ff9800';
  
  fetchJson(`${API_BASE}?action=walkStart`)
    .then(data => {
      status.innerText = 'Walking started successfully!';
      status.style.color = '#28a745';
    })
    .catch(error => {
      status.innerText = 'Error starting walking';
      status.style.color = '#dc3545';
      console.error('startWalking error:', error);
    });
}

function stopWalking() {
  const status = document.getElementById('walkingStatus');
  status.innerText = 'Stopping walking...';
  status.style.color = '#ff9800';
  
  fetchJson(`${API_BASE}?action=walkStop`)
    .then(data => {
      status.innerText = 'Walking stopped successfully!';
      status.style.color = '#28a745';
    })
    .catch(error => {
      status.innerText = 'Error stopping walking';
      status.style.color = '#dc3545';
      console.error('stopWalking error:', error);
    });
}

// Walking parameter update functions
function updateStepLength() {
  const slider = document.getElementById('stepLengthSlider');
  const value = document.getElementById('stepLengthValue');
  const newValue = slider.value;
  value.textContent = newValue;
  
  // Store the value for use in walk control
  window.walkingParams = window.walkingParams || {};
  window.walkingParams.stepLength = parseInt(newValue);
}

function updateStepHeight() {
  const slider = document.getElementById('stepHeightSlider');
  const value = document.getElementById('stepHeightValue');
  const newValue = slider.value;
  value.textContent = newValue;
  
  // Store the value for use in walk control
  window.walkingParams = window.walkingParams || {};
  window.walkingParams.stepHeight = parseInt(newValue);
}

function updateStepDuration() {
  const slider = document.getElementById('stepDurationSlider');
  const value = document.getElementById('stepDurationValue');
  const newValue = slider.value;
  value.textContent = newValue;
  
  // Store the value for use in walk control
  window.walkingParams = window.walkingParams || {};
  window.walkingParams.stepDuration = parseInt(newValue);
}

function updateWalkHeight() {
  const slider = document.getElementById('walkHeightSlider');
  const value = document.getElementById('walkHeightValue');
  const newValue = slider.value;
  value.textContent = newValue;
  
  // Store the value for use in walk control
  window.walkingParams = window.walkingParams || {};
  window.walkingParams.walkHeight = parseInt(newValue);
}

function updateXOffset() {
  const slider = document.getElementById('xOffsetSlider');
  const value = document.getElementById('xOffsetValue');
  const newValue = parseFloat(slider.value);
  value.textContent = newValue.toFixed(1);
  
  // Store the value for use in walk control
  window.walkingParams = window.walkingParams || {};
  window.walkingParams.xOffset = newValue;
}

function applyPresetPosition() {
  const presetSelect = document.getElementById('presetSelect');
  const status = document.getElementById('presetStatus');
  const selectedPreset = presetSelect.value;
  
  status.innerText = 'Applying preset position...';
  status.style.color = '#ff9800';
  
  fetchJson(`${API_BASE}?action=applyPresetPosition&name=${selectedPreset}`)
    .then(data => {
      status.innerText = 'Preset position applied successfully!';
      status.style.color = '#28a745';
    })
    .catch(error => {
      status.innerText = 'Error applying preset position';
      status.style.color = '#dc3545';
      console.error('applyPresetPosition error:', error);
    });
}



// Make all debug functions globally available
window.toggleDriftCompensation = toggleDriftCompensation;
window.resetDriftCompensation = resetDriftCompensation;
window.startWalking = startWalking;
window.stopWalking = stopWalking;
window.applyPresetPosition = applyPresetPosition;

// Make walking parameter functions globally available
window.updateStepLength = updateStepLength;
window.updateStepHeight = updateStepHeight;
window.updateStepDuration = updateStepDuration;
window.updateWalkHeight = updateWalkHeight;
window.updateXOffset = updateXOffset;

// ===== IK SECTION =====
let ikCanvas, ikCtx;
let lastIKResult = null;

function initIKCanvas() {
  ikCanvas = document.getElementById('ikCanvas');
  ikCtx = ikCanvas.getContext('2d');
  
  // Set up event listeners for inputs
  document.getElementById('x').addEventListener('input', function() {
    callInverseKinematics();
  });
  document.getElementById('y').addEventListener('input', function() {
    callInverseKinematics();
  });
  
  // Add click event listener to the canvas
  ikCanvas.addEventListener('click', handleCanvasClick);
  
  // Initial draw only (no API call until robot is connected)
  updateIKVisualization();
}

function handleCanvasClick(event) {
  // Get click coordinates relative to the canvas
  const rect = ikCanvas.getBoundingClientRect();
  const clickX = event.clientX - rect.left;
  const clickY = event.clientY - rect.top;
  
  // Convert canvas coordinates to IK coordinates
  const centerX = ikCanvas.width / 2;
  const centerY = 0;
  const scale = 2; // Must match the scale in updateIKVisualization
  
  // Calculate IK coordinates (accounting for the coordinate system)
  const x = (clickX - centerX) / scale;
  const y = clickY / scale; // y-axis pointing down
  
  // Update input fields
  document.getElementById('x').value = Math.round(x);
  document.getElementById('y').value = Math.round(y);
  
  // Call inverse kinematics
  callInverseKinematics();
}

function updateIKVisualization() {
  if (!ikCtx) return;
  
  // Clear canvas
  ikCtx.clearRect(0, 0, ikCanvas.width, ikCanvas.height);
  
  // Get input values
  const x = parseFloat(document.getElementById('x').value) || 0;
  const y = parseFloat(document.getElementById('y').value) || 82;
  
  // Set up coordinate system (origin at top center, y-axis pointing down)
  const centerX = ikCanvas.width / 2;
  const centerY = 0; // Origin at the top
  const scale = 2; // pixels per unit (scaled by 2)
  
  // Draw coordinate axes
  ikCtx.beginPath();
  ikCtx.moveTo(0, 0);
  ikCtx.lineTo(ikCanvas.width, 0);
  ikCtx.moveTo(centerX, 0);
  ikCtx.lineTo(centerX, ikCanvas.height);
  ikCtx.strokeStyle = '#ddd';
  ikCtx.stroke();
  
  // Draw origin (first pivot)
  ikCtx.beginPath();
  ikCtx.arc(centerX, centerY, 5, 0, Math.PI * 2);
  ikCtx.fillStyle = '#333';
  ikCtx.fill();
  
  // Draw target point
  const targetX = centerX + x * scale;
  const targetY = centerY + y * scale; // y-axis pointing down
  
  ikCtx.beginPath();
  ikCtx.arc(targetX, targetY, 5, 0, Math.PI * 2);
  ikCtx.fillStyle = 'red';
  ikCtx.fill();
  
  // Calculate IK angles (simplified)
  const targetDistance = Math.sqrt(x * x + y * y);
  const l1 = 37;
  const l2 = 45;
  const maxReach = l1 + l2;
  
  // If we have IK results from the server, use those
  if (lastIKResult) {
    // apply offset to the result
    lastIKResult.theta1 += 0;
    lastIKResult.theta2 -= 90;

    const theta1 = lastIKResult.theta1 * Math.PI / 180;
    const theta2 = lastIKResult.theta2 * Math.PI / 180;
    
    // Draw first segment
    const joint1X = centerX + l1 * Math.cos(theta1) * scale;
    const joint1Y = centerY + l1 * Math.sin(theta1) * scale;
    
    ikCtx.beginPath();
    ikCtx.moveTo(centerX, centerY);
    ikCtx.lineTo(joint1X, joint1Y);
    ikCtx.strokeStyle = 'blue';
    ikCtx.lineWidth = 3;
    ikCtx.stroke();
    
    // Draw second segment
    const joint2X = joint1X + l2 * Math.cos(theta1 + theta2) * scale;
    const joint2Y = joint1Y + l2 * Math.sin(theta1 + theta2) * scale;
    
    ikCtx.beginPath();
    ikCtx.moveTo(joint1X, joint1Y);
    ikCtx.lineTo(joint2X, joint2Y);
    ikCtx.strokeStyle = 'green';
    ikCtx.lineWidth = 3;
    ikCtx.stroke();
    
    // Draw joints
    ikCtx.beginPath();
    ikCtx.arc(joint1X, joint1Y, 4, 0, Math.PI * 2);
    ikCtx.fillStyle = '#333';
    ikCtx.fill();
    
    ikCtx.beginPath();
    ikCtx.arc(joint2X, joint2Y, 4, 0, Math.PI * 2);
    ikCtx.fillStyle = '#333';
    ikCtx.fill();
    
    // remove the offset
    lastIKResult.theta1 -= 0;
    lastIKResult.theta2 += 90;

    // Display angles
    ikCtx.fillStyle = '#333';
    ikCtx.font = '12px Arial';
    ikCtx.fillText('θ1: ' + lastIKResult.theta1.toFixed(1) + '°', 10, 20);
    ikCtx.fillText('θ2: ' + lastIKResult.theta2.toFixed(1) + '°', 10, 40);         
  } 
}

function callInverseKinematics() {
  var x = document.getElementById('x').value;
  var y = document.getElementById('y').value;
  
  // Update status to show we're calculating
  document.getElementById('ikStatus').innerText = "Calculating...";
  document.getElementById('ikStatus').style.color = "#ff9800";
  
  fetchJson(`${API_BASE}?action=inverseKinematics&x=${x}&y=${y}`)
    .then(data => {
      lastIKResult = data;
      
      // Check if the target was reached
      if (!lastIKResult.success) {
        // Target was out of reach, show a message
        document.getElementById('ikStatus').innerText = "Target out of reach";
        document.getElementById('ikStatus').style.color = "orange";
      } else {
        // Target was reached
        document.getElementById('ikStatus').innerText = "Target reached - Robot moved!";
        document.getElementById('ikStatus').style.color = "green";
      }
      
      updateIKVisualization();
    })
    .catch(error => {
      document.getElementById('ikStatus').innerText = "Error calculating IK";
      document.getElementById('ikStatus').style.color = "#dc3545";
      console.error('callInverseKinematics error:', error);
    });
}

// Make the callInverseKinematics function globally available
window.callInverseKinematics = callInverseKinematics;

// ===== POSE SECTION =====
function setPose(height, pitch, roll) {
  document.getElementById('heightSlider').value = height;
  document.getElementById('heightValue').innerText = height + 'mm';
  document.getElementById('pitchSlider').value = pitch;
  document.getElementById('pitchValue').innerText = pitch + '°';
  document.getElementById('rollSlider').value = roll;
  document.getElementById('rollValue').innerText = roll + '°';
  setCurrentPose();
}

// Throttled pose setting function
function throttledSetPose() {
  const now = Date.now();
  
  // If we're within the throttle window, schedule a request
  if (now - lastPoseRequest < 100) {
    if (!pendingPoseRequest) {
      pendingPoseRequest = true;
      setTimeout(() => {
        pendingPoseRequest = false;
        setCurrentPose();
      }, 100 - (now - lastPoseRequest));
    }
    return;
  }
  
  // Otherwise, make the request immediately
  lastPoseRequest = now;
  setCurrentPose();
}

function setCurrentPose() {
  const height = lastPoseValues.height;
  const pitch = lastPoseValues.pitch;
  const roll = lastPoseValues.roll;
  
  const poseStatus = document.getElementById('poseStatus');
  
  fetchJson(`${API_BASE}?action=setRobotPose&height=${height}&pitch=${pitch}&roll=${roll}`)
    .then(data => {
      if (poseStatus) {
        poseStatus.innerText = 'Pose set successfully! H:' + height + 'mm, P:' + pitch.toFixed(1) + '°, R:' + roll.toFixed(1) + '°';
        poseStatus.style.color = '#28a745';
      }
    })
    .catch(error => {
      if (poseStatus) {
        poseStatus.innerText = 'Error setting pose';
        poseStatus.style.color = '#dc3545';
      }
      console.error('setCurrentPose error:', error);
    });
}

// Pose control variables
let currentPitch = 0;
let currentRoll = 0;
let lastPoseRequest = 0;
let pendingPoseRequest = false;
let lastPoseValues = { height: 66, pitch: 0, roll: 0 };

// Function to handle height slider changes
function onHeightChange() {
  const height = parseFloat(document.getElementById('heightSlider').value);
  document.getElementById('heightValue').innerText = height + 'mm';
  lastPoseValues.height = height;
  throttledSetPose();
}

// Function to handle pose joystick
function initPoseJoystick() {
  const joystickContainer = document.getElementById('poseJoystick');
  
  if (!joystickContainer) return;
  
  // Create nipple.js joystick
  const poseJoystick = nipplejs.create({
    zone: joystickContainer,
    mode: 'static',
    position: { left: '50%', top: '50%' },
    color: '#4caf50',
    size: 80,
    lockX: false,
    lockY: false,
    dynamicPage: true
  });
  
  poseJoystick.on('move', (evt, data) => {
    // Calculate pitch and roll values (-15 to +15 degrees)
    // data.vector.x and data.vector.y are between -1 and 1
    currentPitch = -data.vector.y * 15; // Y-axis controls pitch (inverted)
    currentRoll = data.vector.x * 15;   // X-axis controls roll
    
    // Update display
    document.getElementById('pitchValue').innerText = currentPitch.toFixed(1) + '°';
    document.getElementById('rollValue').innerText = currentRoll.toFixed(1) + '°';
    
    // Update last pose values
    lastPoseValues.pitch = currentPitch;
    lastPoseValues.roll = currentRoll;
    
    // Send pose to robot
    throttledSetPose();
  });
  
  poseJoystick.on('end', () => {
    // Reset to center
    currentPitch = 0;
    currentRoll = 0;
    
    // Update display
    document.getElementById('pitchValue').innerText = '0°';
    document.getElementById('rollValue').innerText = '0°';
    
    // Update last pose values
    lastPoseValues.pitch = 0;
    lastPoseValues.roll = 0;
    
    // Send pose to robot
    throttledSetPose();
  });
}

// Function to toggle between debug/legs view and raw data view
function toggleDataView() {
  const showRawData = document.getElementById('showRawData').checked;
  const debugSection = document.getElementById('debugSection');
  const legsSection = document.getElementById('legsSection');
  const rawDataSection = document.getElementById('rawDataSection');
  const rawDataContainer = document.getElementById('rawDataContainer');
  
  if (showRawData) {
    // Hide debug and legs sections
    debugSection.style.display = 'none';
    legsSection.style.display = 'none';
    
    // Show raw data with full height
    rawDataSection.style.display = 'block';
    rawDataContainer.style.maxHeight = '400px';
  } else {
    // Show debug and legs sections
    debugSection.style.display = 'block';
    legsSection.style.display = 'block';
    
    // Hide raw data section
    rawDataSection.style.display = 'none';
  }
}

// Make pose functions globally available
window.setPose = setPose;
window.setCurrentPose = setCurrentPose;
window.onHeightChange = onHeightChange;
window.initPoseJoystick = initPoseJoystick;
window.toggleDataView = toggleDataView;

// ===== PRESET POSITIONS SECTION =====
function updatePresetPositions(presetPositions) {
  const select = document.getElementById('presetSelect');
  if (!select) return;
  
  // Only update if the dropdown is empty or has loading text
  if (select.options.length <= 1) {
    // Clear existing options
    select.innerHTML = '';
    
    // Add options for each preset position
    presetPositions.forEach(preset => {
      const option = document.createElement('option');
      option.value = preset;
      option.textContent = preset.charAt(0).toUpperCase() + preset.slice(1) + ' Position';
      select.appendChild(option);
    });
    

  }
}

// ===== RAW DATA SECTION =====
function updateRawData(data) {
  const container = document.getElementById('rawDataContainer');
  if (!container) return;
  
  // Only update raw data if the checkbox is checked
  const showRawData = document.getElementById('showRawData');
  if (!showRawData || !showRawData.checked) return;
  
  // Format the data as JSON with proper indentation
  const formattedData = JSON.stringify(data, null, 2);
  
  // Replace newlines with <br> tags and spaces with &nbsp; for proper display
  const htmlData = formattedData
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/\\n/g, '<br>')
    .replace(/ /g, '&nbsp;');
  
  container.innerHTML = '<pre>' + htmlData + '</pre>';
}

// ===== INITIALIZATION =====
document.addEventListener('DOMContentLoaded', function() {
  
  // Hide all sections initially and show loader
  const sectionsContainer = document.getElementById('sections-container');
  const loader = document.getElementById('loader');
  
  if (sectionsContainer) sectionsContainer.style.display = 'none';
  if (loader) {
    loader.classList.add('visible');
    loader.querySelector('.loader-text').innerText = 'Connecting to robot...';
  }
  
  // Initialize components (but don't show them yet)
  initIKCanvas();
  initPoseJoystick();
  
  // Set initial data view state (debug/legs visible, raw data hidden)
  toggleDataView();
  
  // Initialize joystick immediately
  const joystickContainer = document.getElementById('joystickContainer');
  if (joystickContainer) {
    
    // Ensure container has proper positioning
    joystickContainer.style.position = 'relative';
    joystickContainer.style.overflow = 'hidden';
    
    var joystick = nipplejs.create({
      zone: joystickContainer,
      mode: 'static',
      position: { left: '50%', top: '50%' },
      color: 'blue',
      size: 100,
      lockX: false,
      lockY: false,
      dynamicPage: true
    });
    
    // Handle joystick movements
    joystick.on('move', function (evt, data) {
      if (data && data.vector) {
        var x = Math.round(data.vector.x * 100); // Scale to -100 to 100
        var y = Math.round(data.vector.y * 100); // Scale to -100 to 100
        controlRobotWalk(x, y);
      }
    });
    
    joystick.on('end', function () {
      controlRobotWalk(0, 0); // Stop walking when joystick is released
    });
    
    joystick.on('start', function () {
    });
  } else {
    console.error('Joystick container not found!');
  }
  
  // Add event listener for data updates
  document.addEventListener('robotDataUpdate', function(event) {
    if (event.detail) {
      
      // Show sections on first successful data update
      if (sectionsContainer && sectionsContainer.style.display === 'none') {
        sectionsContainer.style.display = 'flex';
        if (loader) loader.classList.remove('visible');
        
        // Make initial IK calculation after robot is connected
        setTimeout(() => {
          callInverseKinematics();
        }, 100);
      }
      
      // Update legs data
      if (event.detail.servogroup) {
        updateServoValues('legs', event.detail.servogroup);
      }
      

      
      // Update preset positions dropdown
      if (event.detail.presetPositions) {
        updatePresetPositions(event.detail.presetPositions);
      }
      
      // Update raw data
      updateRawData(event.detail);
    }
  });
  
  // Start data polling
  getRobot();
  setInterval(getRobot, 200);
});