#ifndef WEB_PAGE_H
#define WEB_PAGE_H

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Smart Thermometer</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    @import url('https://fonts.googleapis.com/css2?family=Inter:wght@400;700;900&display=swap');
    :root {
      --bg-color: #1a1a2e; --card-color: #16213e; --secondary-text: #a7a9be;
      --color-low: #00bfff; --color-normal: #2ecc71; --color-elevated: #f39c12; --color-fever: #e74c3c;
    }
    body {
      font-family: 'Inter', sans-serif; display: flex; flex-direction: column; align-items: center; justify-content: center; min-height: 100vh;
      margin: 0; padding: 20px; color: var(--secondary-text); box-sizing: border-box;
      background: linear-gradient(-45deg, #1a1a2e, #16213e, #0f3460, #1a1a2e);
      background-size: 400% 400%;
      animation: gradientBG 15s ease infinite;
    }
    @keyframes gradientBG {
        0% { background-position: 0% 50%; }
        50% { background-position: 100% 50%; }
        100% { background-position: 0% 50%; }
    }
    .container {
      width: 100%; max-width: 500px; text-align: center;
      background: rgba(0,0,0,0.2);
      padding: 30px;
      border-radius: 20px;
      backdrop-filter: blur(10px);
      border: 1px solid rgba(255,255,255,0.1);
    }
    .temp-display {
      width: 250px; height: 250px; border-radius: 50%; margin: 0 auto 30px;
      display: flex; flex-direction: column; justify-content: center; align-items: center;
      background: radial-gradient(circle, var(--card-color) 60%, transparent 80%);
      border: 5px solid; transition: border-color 0.5s ease, box-shadow 0.5s ease;
    }
    .temp-value { font-size: 5rem; font-weight: 900; line-height: 1; transition: color 0.5s ease; }
    .temp-status { font-size: 1.5rem; font-weight: 700; margin-top: 10px; transition: color 0.5s ease; }
    .health-key { display: flex; flex-wrap: wrap; justify-content: center; gap: 15px; margin-top: 30px; padding-top: 20px; border-top: 1px solid rgba(255,255,255,0.1); }
    .key-item { display: flex; align-items: center; gap: 8px; font-size: 0.9rem;}
    .key-color { width: 15px; height: 15px; border-radius: 50%; }
  </style>
</head>
<body>
<div class="container">
    <div class="temp-display" id="temp-display">
        <div class="temp-value" id="temp-value">--.-&deg;C</div>
        <div class="temp-status" id="temp-status">LOADING...</div>
    </div>
    <div class="health-key">
        <div class="key-item"><div class="key-color" style="background-color: var(--color-low);"></div> Low</div>
        <div class="key-item"><div class="key-color" style="background-color: var(--color-normal);"></div> Normal</div>
        <div class="key-item"><div class="key-color" style="background-color: var(--color-elevated);"></div> Elevated</div>
        <div class="key-item"><div class="key-color" style="background-color: var(--color-fever);"></div> Fever</div>
    </div>
</div>

<script>
function updateTemperatureUI(temp, status) {
    const tempValueEl = document.getElementById('temp-value');
    const tempStatusEl = document.getElementById('temp-status');
    const displayCircle = document.getElementById('temp-display');
    
    tempValueEl.innerHTML = `${parseFloat(temp).toFixed(1)}&deg;C`;
    tempStatusEl.textContent = status;

    let colorVar;
    if (status === 'LOW') colorVar = 'var(--color-low)';
    else if (status === 'NORMAL') colorVar = 'var(--color-normal)';
    else if (status === 'ELEVATED') colorVar = 'var(--color-elevated)';
    else if (status === 'FEVER') colorVar = 'var(--color-fever)';
    
    displayCircle.style.borderColor = colorVar;
    displayCircle.style.boxShadow = `0 0 25px ${colorVar}`;
    tempStatusEl.style.color = colorVar;
    tempValueEl.style.color = colorVar;
}

function fetchData() {
    fetch('/data')
        .then(response => response.json())
        .then(data => {
            const temp = parseFloat(data.temperature);
            updateTemperatureUI(temp, data.status);
        }).catch(error => console.error('Error fetching data:', error));
}

window.onload = () => {
    fetchData(); // Fetch initial data
    setInterval(fetchData, 3000); // Refresh every 3 seconds
};
</script>
</body>
</html>
)rawliteral";

#endif // WEB_PAGE_H
