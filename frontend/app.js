const API_URL = 'http://localhost:8080/api';
let currentCardNo = null;

// Load saved session on page load
window.addEventListener('load', () => {
    const savedCard = localStorage.getItem('atm_cardNo');
    if (savedCard) {
        currentCardNo = parseInt(savedCard);
        // Verify session is still valid
        fetch(`${API_URL}/balance?card=${currentCardNo}`)
            .then(res => res.json())
            .then(data => {
                if (data.success) {
                    updateDashboardBalance();
loadRecentTransactions();
showScreen('dashboardScreen');
                    showToast('Session restored!', 'success');
                } else {
                    localStorage.removeItem('atm_cardNo');
                    currentCardNo = null;
                }
            })
            .catch(() => {
                localStorage.removeItem('atm_cardNo');
                currentCardNo = null;
            });
    }
});

// Background slideshow
let currentSlide = 0;
const slides = document.querySelectorAll('.slide');
if (slides.length > 0) {
    setInterval(() => {
        slides[currentSlide].classList.remove('active');
        currentSlide = (currentSlide + 1) % slides.length;
        slides[currentSlide].classList.add('active');
    }, 5000);
}

function playCashSound() {
    try {
        const audioContext = new (window.AudioContext || window.webkitAudioContext)();
        const oscillator = audioContext.createOscillator();
        const gainNode = audioContext.createGain();
        oscillator.connect(gainNode);
        gainNode.connect(audioContext.destination);
        oscillator.frequency.value = 880;
        gainNode.gain.value = 0.15;
        oscillator.start();
        gainNode.gain.exponentialRampToValueAtTime(0.00001, audioContext.currentTime + 0.5);
        oscillator.stop(audioContext.currentTime + 0.3);
    } catch(e) {}
}

function showNotesAnimation(amount, isDeposit = false) {
    const container = document.getElementById('notesContainer');
    if (!container) return;
    const noteCount = Math.min(Math.floor(amount / 100), 15);
    for (let i = 0; i < noteCount; i++) {
        setTimeout(() => {
            const note = document.createElement('div');
            note.className = 'note';
            note.innerHTML = isDeposit ? '💰' : '💵';
            note.style.left = Math.random() * window.innerWidth + 'px';
            note.style.top = window.innerHeight - 100 + 'px';
            note.style.fontSize = (25 + Math.random() * 20) + 'px';
            note.style.setProperty('--tx', (Math.random() * 200 - 100) + 'px');
            note.style.setProperty('--ty', '-' + (100 + Math.random() * 200) + 'px');
            container.appendChild(note);
            setTimeout(() => note.remove(), 800);
        }, i * 50);
    }
    playCashSound();
}

function showToast(msg, type) {
    const container = document.getElementById('toastContainer');
    if (!container) return;
    const toast = document.createElement('div');
    toast.className = `toast ${type}`;
    toast.innerHTML = `<i class="fas ${type === 'success' ? 'fa-check-circle' : 'fa-exclamation-circle'}"></i> ${msg}`;
    container.appendChild(toast);
    setTimeout(() => toast.remove(), 3000);
}

function fillDemo(cardNo, pin) {
    document.getElementById('cardNo').value = cardNo;
    document.getElementById('pin').value = pin;
}

async function updateDashboardBalance() {
    if (!currentCardNo) return;
    try {
        const response = await fetch(`${API_URL}/balance?card=${currentCardNo}`);
        const data = await response.json();
        if (data.success) {
            const balanceEl = document.getElementById('dashboardBalance');
            if (balanceEl) {
                balanceEl.textContent = `₹${data.balance.toFixed(2)}`;
            }
        }
    } catch (error) {
        console.log('Balance update error:', error);
    }
}

function showScreen(screenId) {
    const screen = document.getElementById(screenId);
    if (!screen) {
        console.error("Screen not found:", screenId);
        return;
    }
    document.querySelectorAll('.screen').forEach(s => s.classList.remove('active'));
    screen.classList.add('active');
}

function showDashboard() {
    showScreen('dashboardScreen');
    if (currentCardNo) {
        updateDashboardBalance();
        loadRecentTransactions();
    }
}

async function login() {
    const cardNo = document.getElementById('cardNo').value;
    const pin = document.getElementById('pin').value;
    if (!cardNo || !pin) {
        showToast('Please enter card number and PIN', 'error');
        return;
    }
    try {
        const response = await fetch(`${API_URL}/login`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ cardNo: parseInt(cardNo), pin: parseInt(pin) })
        });
        const data = await response.json();
        if (data.success) {
            currentCardNo = data.cardNo;
            // Save to localStorage
            localStorage.setItem('atm_cardNo', currentCardNo);
            
            let displayName = data.name;
            
            const userNameEl = document.getElementById('userName');
            const userCardEl = document.getElementById('userCard');
            const dashboardBalanceEl = document.getElementById('dashboardBalance');
            
            if (userNameEl) userNameEl.textContent = displayName;
            if (userCardEl) userCardEl.textContent = `**** ${data.cardNo}`;
            if (dashboardBalanceEl) dashboardBalanceEl.textContent = `₹${data.balance.toFixed(2)}`;
            
            showToast(`Welcome ${displayName}!`, 'success');
            updateDashboardBalance();
loadRecentTransactions();
showScreen('dashboardScreen');
            document.getElementById('cardNo').value = '';
            document.getElementById('pin').value = '';
        } else {
            showToast(data.message, 'error');
        }
    } catch (error) {
        showToast('Cannot connect to server! Make sure backend is running', 'error');
    }
}

function logout() {
    currentCardNo = null;
    localStorage.removeItem('atm_cardNo');
    showScreen('loginScreen');
    showToast('Logged out successfully', 'success');
}

async function showBalance() {
    if (!currentCardNo) return;
    try {
        const response = await fetch(`${API_URL}/balance?card=${currentCardNo}`);
        const data = await response.json();
        if (data.success) {
            const balanceAmountEl = document.getElementById('balanceAmount');
            if (balanceAmountEl) {
                balanceAmountEl.textContent = `₹${data.balance.toFixed(2)}`;
            }
            showScreen('balanceScreen');
        } else {
            showToast(data.message, 'error');
        }
    } catch (error) {
        showToast('Cannot connect to server', 'error');
    }
}

function showWithdraw() {
    const withdrawAmount = document.getElementById('withdrawAmount');
    if (withdrawAmount) withdrawAmount.value = '';
    showScreen('withdrawScreen');
}

function setAmount(amount) {
    const withdrawAmount = document.getElementById('withdrawAmount');
    if (withdrawAmount) withdrawAmount.value = amount;
}

async function withdraw() {
    if (!currentCardNo) {
        showToast('Session expired! Please login again', 'error');
        logout();
        return;
    }
    const amountInput = document.getElementById('withdrawAmount');
    if (!amountInput) return;
    
    const amount = parseFloat(amountInput.value);
    if (!amount || amount <= 0) {
        showToast('Please enter valid amount', 'error');
        return;
    }
    if (amount % 100 !== 0) {
        showToast('Amount must be in multiples of 100', 'error');
        return;
    }
    try {
        const response = await fetch(`${API_URL}/withdraw`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ cardNo: currentCardNo, amount: amount })
        });
        const data = await response.json();
        if (data.success) {
            showNotesAnimation(amount, false);
            showToast(`✅ ₹${amount} withdrawn! New balance: ₹${data.newBalance}`, 'success');
            
            const dashboardBalance = document.getElementById('dashboardBalance');
            if (dashboardBalance) {
                dashboardBalance.textContent = `₹${data.newBalance.toFixed(2)}`;
            }
            
            // Stay on dashboard
            showScreen('dashboardScreen');
            loadRecentTransactions();
        } else {
            showToast(data.message, 'error');
        }
    } catch (error) {
        console.error('Withdraw error:', error);
        showToast('Transaction failed! Check if server is running', 'error');
    }
}

function showDeposit() {
    const depositAmount = document.getElementById('depositAmount');
    if (depositAmount) depositAmount.value = '';
    showScreen('depositScreen');
}

function setDepositAmount(amount) {
    const depositAmount = document.getElementById('depositAmount');
    if (depositAmount) depositAmount.value = amount;
}

async function deposit() {
    if (!currentCardNo) {
        showToast('Session expired! Please login again', 'error');
        logout();
        return;
    }
    const amountInput = document.getElementById('depositAmount');
    if (!amountInput) return;
    
    const amount = parseFloat(amountInput.value);
    if (!amount || amount <= 0) {
        showToast('Please enter valid amount', 'error');
        return;
    }
    try {
        const response = await fetch(`${API_URL}/deposit`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ cardNo: currentCardNo, amount: amount })
        });
        const data = await response.json();
        if (data.success) {
            showNotesAnimation(amount, true);
            showToast(`✅ ₹${amount} deposited! New balance: ₹${data.newBalance}`, 'success');
            
            const dashboardBalance = document.getElementById('dashboardBalance');
            if (dashboardBalance) {
                dashboardBalance.textContent = `₹${data.newBalance.toFixed(2)}`;
            }
            
            // Stay on dashboard
            showScreen('dashboardScreen');
            loadRecentTransactions();
        } else {
            showToast(data.message, 'error');
        }
    } catch (error) {
        console.error('Deposit error:', error);
        showToast('Transaction failed! Check if server is running', 'error');
    }
}

async function showHistory() {
    if (!currentCardNo) return;
    const historyDiv = document.getElementById('historyList');
    if (!historyDiv) return;
    
    historyDiv.innerHTML = '<div class="loading-row"><i class="fas fa-spinner fa-spin"></i> Loading...</div>';
    showScreen('historyScreen');
    try {
        const response = await fetch(`${API_URL}/history?card=${currentCardNo}`);
        const data = await response.json();
        if (data.transactions && data.transactions.length > 0) {
            historyDiv.innerHTML = data.transactions.slice(0, 20).map(t => {
                let icon = t.type === 'Deposit' ? 'fa-arrow-down' : (t.type === 'Withdraw' ? 'fa-arrow-up' : 'fa-key');
                let color = t.type === 'Deposit' ? '#22c55e' : (t.type === 'Withdraw' ? '#ef4444' : '#C5A059');
                let sign = t.type === 'Deposit' ? '+' : (t.type === 'Withdraw' ? '-' : '');
                return `
                    <div class="history-item-luxury">
                        <i class="fas ${icon}" style="color:${color}"></i>
                        <div class="history-details">
                            <strong>${t.type}</strong>
                            <small>${t.date}</small>
                        </div>
                        <div class="history-amount" style="color:${color}">${sign} ₹${t.amount.toFixed(2)}</div>
                        <div class="history-balance">₹${t.balanceAfter.toFixed(2)}</div>
                    </div>
                `;
            }).join('');
        } else {
            historyDiv.innerHTML = '<div class="loading-row">No transactions found</div>';
        }
    } catch (error) {
        historyDiv.innerHTML = '<div class="loading-row">Error loading transactions</div>';
    }
}

async function loadRecentTransactions() {
    if (!currentCardNo) return;
    const container = document.getElementById('recentTransactions');
    if (!container) return;
    try {
        const response = await fetch(`${API_URL}/history?card=${currentCardNo}`);
        const data = await response.json();
        if (data.transactions && data.transactions.length > 0) {
            const recent = data.transactions.slice(0, 5);
            container.innerHTML = recent.map(t => {
                let icon = t.type === 'Deposit' ? 'fa-arrow-down' : 'fa-arrow-up';
                let color = t.type === 'Deposit' ? '#22c55e' : '#ef4444';
                let sign = t.type === 'Deposit' ? '+' : '-';
                return `
                    <div class="recent-item">
                        <div class="recent-type">
                            <i class="fas ${icon}" style="color:${color}"></i>
                            <span>${t.type}</span>
                        </div>
                        <div class="recent-amount" style="color:${color}">${sign} ₹${t.amount.toFixed(2)}</div>
                        <div class="recent-date">${t.date.split(' ')[0]}</div>
                    </div>
                `;
            }).join('');
        } else {
            container.innerHTML = '<div class="loading-row">No recent transactions</div>';
        }
    } catch (error) {
        container.innerHTML = '<div class="loading-row">Unable to load transactions</div>';
    }
}

function showChangePin() {
    const oldPin = document.getElementById('oldPin');
    const newPin = document.getElementById('newPin');
    const confirmPin = document.getElementById('confirmPin');
    
    if (oldPin) oldPin.value = '';
    if (newPin) newPin.value = '';
    if (confirmPin) confirmPin.value = '';
    showScreen('pinScreen');
}

async function changePin() {
    if (!currentCardNo) return;
    const oldPin = document.getElementById('oldPin').value;
    const newPin = document.getElementById('newPin').value;
    const confirmPin = document.getElementById('confirmPin').value;
    
    if (!oldPin || !newPin || !confirmPin) {
        showToast('Please fill all fields', 'error');
        return;
    }
    if (oldPin.length !== 4 || newPin.length !== 4) {
        showToast('PIN must be 4 digits', 'error');
        return;
    }
    if (newPin !== confirmPin) {
        showToast('New PIN and confirm PIN do not match', 'error');
        return;
    }
    if (oldPin === newPin) {
        showToast('New PIN must be different', 'error');
        return;
    }
    try {
        const response = await fetch(`${API_URL}/changepin`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ cardNo: currentCardNo, oldPin: parseInt(oldPin), newPin: parseInt(newPin) })
        });
        const data = await response.json();
        if (data.success) {
            showToast('PIN changed! Please login again', 'success');
            logout();
        } else {
            showToast(data.message, 'error');
        }
    } catch (error) {
        showToast('Cannot connect to server', 'error');
    }
}

document.addEventListener('keypress', (e) => {
    if (e.key === 'Enter' && document.getElementById('loginScreen') && document.getElementById('loginScreen').classList.contains('active')) {
        login();
    }
});

setInterval(() => {
    if (currentCardNo) updateDashboardBalance();
}, 5000);