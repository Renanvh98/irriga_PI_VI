// Referências aos elementos
const umidadeEl = document.getElementById('umidadeAtual');
const fluxoEl = document.getElementById('fluxoAtual');
const statusEl = document.getElementById('statusValvula');
const btnLigar = document.getElementById('btnLigar');
const btnDesligar = document.getElementById('btnDesligar');
const toggleBtn = document.getElementById('themeToggle');

let umidadeHistory = [];
let vazaoHistory = [];
let labels = [];

let irrigando = false;

// Configuração dos gráficos
const ctxU = document.getElementById('umidadeChart').getContext('2d');
const umidadeChart = new Chart(ctxU, {
  type: 'line',
  data: {
    labels: labels,
    datasets: [{
      label: 'Umidade (%)',
      data: umidadeHistory,
      fill: true,
      tension: 0.3,
      borderColor: '#22c55e',
      backgroundColor: 'rgba(34, 197, 94, 0.2)'
    }]
  }
});

const ctxV = document.getElementById('vazaoChart').getContext('2d');
const vazaoChart = new Chart(ctxV, {
  type: 'bar',
  data: {
    labels: labels,
    datasets: [{
      label: 'Vazão (L/min)',
      data: vazaoHistory,
      backgroundColor: '#3b82f6'
    }]
  }
});

// Simulação de valores
function gerarUmidade() {
  const base = irrigando ? 60 : 40;
  const flutuacao = Math.floor(Math.random() * 10) - 5;
  return Math.max(0, Math.min(100, base + flutuacao));
}

function gerarVazao() {
  return irrigando ? (2.2 + Math.random() * 0.4).toFixed(2) : "0.00";
}

// Atualiza os dados simulados
function atualizarSimulacao() {
  const umidade = gerarUmidade();
  const vazao = parseFloat(gerarVazao());
  const now = new Date().toLocaleTimeString();

  umidadeEl.innerText = umidade + ' %';
  fluxoEl.innerText = vazao.toFixed(2) + ' L/min';
  statusEl.innerText = irrigando ? 'Ligada' : 'Desligada';
  statusEl.style.color = irrigando ? 'green' : 'red';

  labels.push(now);
  umidadeHistory.push(umidade);
  vazaoHistory.push(vazao);

  if (labels.length > 20) {
    labels.shift();
    umidadeHistory.shift();
    vazaoHistory.shift();
  }

  umidadeChart.update();
  vazaoChart.update();

  // Controle automático simples
  if (!irrigando && umidade < 45) {
    irrigando = true;
  } else if (irrigando && umidade > 53) {
    irrigando = false;
  }
}

// Controles manuais
btnLigar.addEventListener('click', () => {
  irrigando = true;
  atualizarSimulacao();
});

btnDesligar.addEventListener('click', () => {
  irrigando = false;
  atualizarSimulacao();
});

// Botão de tema
toggleBtn.addEventListener('click', () => {
  document.body.classList.toggle('dark');
  toggleBtn.textContent = document.body.classList.contains('dark') ? '☀️ Claro' : '🌙 Escuro';
});

// Atualização automática
setInterval(atualizarSimulacao, 10000);
atualizarSimulacao();
