// Chemiss Frontend - WebSocket Client
const ChemissApp = (function() {
    const WS_URL = 'ws://localhost:8080';
    let ws = null;
    let gameState = null;
    let selectedPieceId = null;
    let legalMoves = [];
    let pendingTransmutation = null;
    let lastMove = null;

    // Periodic table layout (simplified for display)
    const periodicTableLayout = [
        ['H','','','','','','','','','','','','','','','','','He'],
        ['Li','Be','','','','','','','','','','','B','C','N','O','F','Ne'],
        ['Na','Mg','','','','','','','','','','','Al','Si','P','S','Cl','Ar'],
        ['K','Ca','','','','','','','','','','','','','','','',''],
        ['','','','','','','','','','','','','','','','','',''],
        ['','','','','','','','','','','','','','','','','',''],
        ['','','','','','','','','','','','','','','','','',''],
        ['','','','','','','','','','','','','','','','','',''],
        ['','','','','','','','','','','','','','','','','',''],
        ['','','','','','','','','','','','','','','','','','']
    ];

    const elementTypes = {
        'H':'nonmetal','He':'noble','Li':'metal','Be':'metal','B':'nonmetal','C':'nonmetal',
        'N':'nonmetal','O':'nonmetal','F':'nonmetal','Ne':'noble','Na':'metal','Mg':'metal',
        'Al':'metal','Si':'nonmetal','P':'nonmetal','S':'nonmetal','Cl':'nonmetal','Ar':'noble',
        'K':'metal','Ca':'metal','Fe':'metal','Cu':'metal','Zn':'metal','Ga':'metal',
        'Ge':'metal','As':'nonmetal','Se':'nonmetal','Br':'nonmetal','Kr':'noble',
        'Rb':'metal','Sr':'metal','Ag':'metal','I':'nonmetal','Xe':'noble',
        'Cs':'metal','Ba':'metal','Au':'metal','Hg':'metal','Pb':'metal','Bi':'metal',
        'Ra':'metal','Th':'metal','U':'metal'
    };

    function init() {
        renderBoard(); // Render empty board immediately
        connectWebSocket();
        setupEventListeners();
    }

    function connectWebSocket() {
        updateConnectionStatus('connecting', '连接服务器中...');
        ws = new WebSocket(WS_URL);
        ws.onopen = () => {
            console.log('Connected to Chemiss server');
            updateConnectionStatus('connected', '已连接');
            requestState();
        };
        ws.onmessage = (event) => {
            try {
                const msg = JSON.parse(event.data);
                handleServerMessage(msg);
            } catch (e) {
                console.error('Invalid message:', event.data);
            }
        };
        ws.onclose = () => {
            console.log('Connection closed, retrying in 2s...');
            updateConnectionStatus('disconnected', '连接断开，2秒后重试...');
            setTimeout(connectWebSocket, 2000);
        };
        ws.onerror = (err) => {
            console.error('WebSocket error:', err);
            updateConnectionStatus('error', '连接失败');
        };
    }

    function updateConnectionStatus(status, text) {
        const el = document.getElementById('connection-status');
        if (!el) return;
        el.textContent = text;
        el.className = 'connection-status ' + status;
    }

    function send(msg) {
        if (ws && ws.readyState === WebSocket.OPEN) {
            ws.send(JSON.stringify(msg));
        }
    }

    function requestState() {
        send({type: 'get_state'});
    }

    function handleServerMessage(msg) {
        if (msg.type === 'state' || msg.type === 'move_result' || 
            msg.type === 'transmute_result' || msg.type === 'donate_result' ||
            msg.type === 'new_game' || msg.type === 'draw_requested' ||
            msg.type === 'draw_accepted') {
            if (msg.state) {
                gameState = msg.state;
                renderBoard();
                updateUI();
            }
        } else if (msg.type === 'legal_moves') {
            if (msg.data) {
                legalMoves = msg.data.moves || [];
                highlightLegalMoves();
            }
        }
    }

    function setupEventListeners() {
        document.getElementById('btn-new-game').addEventListener('click', () => {
            send({type: 'new_game'});
            selectedPieceId = null;
            legalMoves = [];
        });
        document.getElementById('btn-draw').addEventListener('click', () => {
            send({type: 'draw'});
        });
        document.getElementById('btn-accept-draw').addEventListener('click', () => {
            send({type: 'accept_draw'});
        });
        document.getElementById('btn-cancel-transmutation').addEventListener('click', () => {
            document.getElementById('transmutation-modal').style.display = 'none';
            pendingTransmutation = null;
        });
    }

    function renderBoard() {
        const boardEl = document.getElementById('board');
        boardEl.innerHTML = '';
        
        // Always render empty cells (even without gameState)
        for (let r = 0; r < 8; r++) {
            for (let c = 0; c < 8; c++) {
                const cell = document.createElement('div');
                cell.className = 'cell ' + ((r + c) % 2 === 0 ? 'light' : 'dark');
                cell.dataset.row = r;
                cell.dataset.col = c;
                cell.addEventListener('click', () => onCellClick(r, c));
                boardEl.appendChild(cell);
            }
        }
        
        if (!gameState || !gameState.board) return;

        // Place pieces
        const pieces = gameState.board.pieces || [];
        pieces.forEach(piece => {
            if (!piece.alive) return;
            const cell = getCell(piece.row, piece.col);
            if (!cell) return;

            const pieceEl = document.createElement('div');
            pieceEl.className = 'piece ' + (piece.owner === 0 ? 'white' : 'black');
            if (piece.bondType !== 'none') pieceEl.classList.add('bonded');
            if (piece.status === 'stunned') pieceEl.classList.add('stunned');
            if (piece.status === 'radioactive') pieceEl.classList.add('radioactive');
            if (piece.charge > 0) pieceEl.classList.add('cation');
            if (piece.charge < 0) pieceEl.classList.add('anion');
            if (piece.id === selectedPieceId) pieceEl.classList.add('selected');

            pieceEl.innerHTML = `
                <span class="symbol">${piece.symbol}</span>
                <span class="mass">${piece.massNumber}</span>
                <span class="electrons">e:${piece.totalElectrons}</span>
            `;
            pieceEl.dataset.id = piece.id;
            pieceEl.addEventListener('click', (e) => {
                e.stopPropagation();
                onPieceClick(piece.id);
            });

            cell.appendChild(pieceEl);
        });

        // Highlight legal moves
        highlightLegalMoves();
        
        // Highlight last move
        if (lastMove) {
            const fromCell = getCell(lastMove.fromRow, lastMove.fromCol);
            const toCell = getCell(lastMove.toRow, lastMove.toCol);
            if (fromCell) fromCell.classList.add('last-move');
            if (toCell) toCell.classList.add('last-move');
        }
    }

    function getCell(row, col) {
        return document.querySelector(`.cell[data-row="${row}"][data-col="${col}"]`);
    }

    function highlightLegalMoves() {
        document.querySelectorAll('.cell.valid-move').forEach(c => c.classList.remove('valid-move'));
        legalMoves.forEach(m => {
            const cell = getCell(m.row, m.col);
            if (cell) cell.classList.add('valid-move');
        });
    }

    function onPieceClick(pieceId) {
        if (!gameState) return;
        if (gameState.phase !== 'playing') return;
        
        const piece = findPiece(pieceId);
        if (!piece) return;

        // Check if it's this player's turn
        const currentPlayer = gameState.currentPlayer;
        if (piece.owner !== currentPlayer) {
            // If we have a piece selected, try to capture
            if (selectedPieceId !== null) {
                const selectedPiece = findPiece(selectedPieceId);
                if (selectedPiece && selectedPiece.owner === currentPlayer) {
                    tryMove(selectedPieceId, piece.row, piece.col);
                }
            }
            return;
        }

        if (selectedPieceId === pieceId) {
            // Deselect
            selectedPieceId = null;
            legalMoves = [];
            renderBoard();
            clearSelectionInfo();
            return;
        }

        selectedPieceId = pieceId;
        legalMoves = [];
        renderBoard();
        showPieceInfo(piece);
        send({type: 'legal_moves', pieceId: pieceId});
    }

    function onCellClick(row, col) {
        if (!gameState || gameState.phase !== 'playing') return;
        if (selectedPieceId === null) return;
        
        const piece = findPiece(selectedPieceId);
        if (!piece || piece.owner !== gameState.currentPlayer) return;

        tryMove(selectedPieceId, row, col);
    }

    function tryMove(pieceId, row, col) {
        // Check if it's a valid move in our local legal moves
        const isValid = legalMoves.some(m => m.row === row && m.col === col);
        if (!isValid) {
            selectedPieceId = null;
            legalMoves = [];
            renderBoard();
            return;
        }

        lastMove = { fromRow: findPiece(pieceId).row, fromCol: findPiece(pieceId).col, toRow: row, toCol: col };
        selectedPieceId = null;
        legalMoves = [];
        send({type: 'move', pieceId: pieceId, to: {row: row, col: col}});
    }

    function findPiece(id) {
        if (!gameState || !gameState.board) return null;
        return (gameState.board.pieces || []).find(p => p.id === id && p.alive);
    }

    function showPieceInfo(piece) {
        const infoEl = document.getElementById('selected-piece-info');
        const typeStr = piece.type === 'metal' ? '金属' : (piece.type === 'noble' ? '稀有气体' : '非金属');
        const bondStr = piece.bondType === 'none' ? '无' : 
                        (piece.bondType === 'covalent' ? '共价键' : 
                         (piece.bondType === 'metallic' ? '金属键' : '离子键'));
        
        infoEl.innerHTML = `
            <div class="info-row"><span class="info-label">元素</span><span class="info-value">${piece.symbol}</span></div>
            <div class="info-row"><span class="info-label">质量数</span><span class="info-value">${piece.massNumber}</span></div>
            <div class="info-row"><span class="info-label">类型</span><span class="info-value">${typeStr}</span></div>
            <div class="info-row"><span class="info-label">价电子</span><span class="info-value">${piece.valenceElectrons}</span></div>
            <div class="info-row"><span class="info-label">总电子</span><span class="info-value">${piece.totalElectrons}</span></div>
            <div class="info-row"><span class="info-label">电负性</span><span class="info-value">${piece.electronegativity}</span></div>
            <div class="info-row"><span class="info-label">周期</span><span class="info-value">${piece.period}</span></div>
            <div class="info-row"><span class="info-label">电荷</span><span class="info-value">${piece.charge > 0 ? '+' : (piece.charge < 0 ? '-' : '0')}</span></div>
            <div class="info-row"><span class="info-label">键合</span><span class="info-value">${bondStr}</span></div>
            <div class="info-row"><span class="info-label">状态</span><span class="info-value">${piece.status === 'normal' ? '正常' : (piece.status === 'stunned' ? '眩晕' : '放射性')}</span></div>
        `;

        // Check if transmutation is available
        if (piece.symbol === 'Li') {
            const isBackRank = (piece.owner === 0 && piece.row === 0) || (piece.owner === 1 && piece.row === 7);
            if (isBackRank) {
                infoEl.innerHTML += `<div style="margin-top:10px;"><button id="btn-transmute" style="width:100%;padding:8px;">核变</button></div>`;
                setTimeout(() => {
                    const btn = document.getElementById('btn-transmute');
                    if (btn) btn.addEventListener('click', () => openTransmutationModal(piece.id));
                }, 0);
            }
        }

        // Check if electron donation is available
        if (piece.type === 'metal') {
            infoEl.innerHTML += `<div style="margin-top:8px;"><button id="btn-donate" style="width:100%;padding:8px;">给电子</button></div>`;
            setTimeout(() => {
                const btn = document.getElementById('btn-donate');
                if (btn) btn.addEventListener('click', () => startElectronDonation(piece.id));
            }, 0);
        }
    }

    function clearSelectionInfo() {
        document.getElementById('selected-piece-info').innerHTML = '<p>点击棋子查看详情</p>';
    }

    function startElectronDonation(donorId) {
        const donor = findPiece(donorId);
        if (!donor) return;
        
        // Find adjacent friendly nonmetals
        const adjacent = getAdjacentPieces(donor.row, donor.col);
        const recipients = adjacent.filter(p => p.owner === donor.owner && p.type === 'nonmetal' && p.type !== 'noble');
        
        if (recipients.length === 0) {
            alert('没有相邻的己方非金属可接受电子');
            return;
        }
        if (recipients.length === 1) {
            send({type: 'donate', donorId: donorId, recipientId: recipients[0].id});
            return;
        }
        
        // Show selection for multiple recipients
        const choice = prompt('选择接受电子的棋子ID: ' + recipients.map(r => r.symbol + '(' + r.id + ')').join(', '));
        if (choice) {
            const id = parseInt(choice);
            if (!isNaN(id)) {
                send({type: 'donate', donorId: donorId, recipientId: id});
            }
        }
    }

    function getAdjacentPieces(row, col) {
        if (!gameState || !gameState.board) return [];
        const pieces = gameState.board.pieces || [];
        return pieces.filter(p => {
            if (!p.alive) return false;
            const dr = Math.abs(p.row - row);
            const dc = Math.abs(p.col - col);
            return dr <= 1 && dc <= 1 && !(dr === 0 && dc === 0);
        });
    }

    function openTransmutationModal(pieceId) {
        pendingTransmutation = pieceId;
        const modal = document.getElementById('transmutation-modal');
        const table = document.getElementById('periodic-table');
        table.innerHTML = '';
        
        for (let r = 0; r < periodicTableLayout.length; r++) {
            for (let c = 0; c < 18; c++) {
                const symbol = (periodicTableLayout[r] && periodicTableLayout[r][c]) || '';
                const el = document.createElement('div');
                if (!symbol) {
                    el.className = 'pt-element empty';
                } else {
                    const type = elementTypes[symbol] || 'nonmetal';
                    el.className = 'pt-element ' + type + (symbol === 'H' ? ' disabled' : '');
                    el.textContent = symbol;
                    el.title = symbol;
                    if (symbol !== 'H') {
                        el.addEventListener('click', () => selectElementForTransmutation(symbol));
                    }
                }
                table.appendChild(el);
            }
        }
        
        document.getElementById('isotope-selector').style.display = 'none';
        modal.style.display = 'flex';
    }

    function selectElementForTransmutation(element) {
        // In a real implementation, we'd fetch isotopes from the server
        // For now, just send with default mass number
        send({type: 'transmute', pieceId: pendingTransmutation, element: element, massNumber: 0});
        document.getElementById('transmutation-modal').style.display = 'none';
        pendingTransmutation = null;
    }

    function updateUI() {
        if (!gameState) return;

        document.getElementById('turn-indicator').textContent = 
            gameState.currentPlayer === 0 ? '白方回合' : '黑方回合';
        document.getElementById('turn-number').textContent = '第 ' + gameState.turnNumber + ' 回合';
        document.getElementById('game-status').textContent = 
            gameState.phase === 'gameover' ? '已结束' : '进行中';

        const acceptDrawBtn = document.getElementById('btn-accept-draw');
        if (gameState.result === 'ongoing' && gameState.drawRequested) {
            acceptDrawBtn.style.display = 'inline-block';
        } else {
            acceptDrawBtn.style.display = 'none';
        }

        // Update bonds list
        const bondsEl = document.getElementById('bonds-list');
        if (gameState.board && gameState.board.pieces) {
            const bonded = gameState.board.pieces.filter(p => p.alive && p.bondType !== 'none');
            if (bonded.length === 0) {
                bondsEl.innerHTML = '<p style="color:#888;font-size:0.85rem;">暂无键合</p>';
            } else {
                bondsEl.innerHTML = bonded.map(p => {
                    const partnerIds = p.bondedTo || [];
                    const partners = partnerIds.map(id => {
                        const partner = findPiece(id);
                        return partner ? partner.symbol : '?';
                    }).join(',');
                    const bondType = p.bondType === 'covalent' ? '共价' : (p.bondType === 'metallic' ? '金属' : '离子');
                    return `<div style="padding:3px 0;font-size:0.85rem;">${p.symbol} - ${bondType}[${partners}]</div>`;
                }).join('');
            }
        }

        // Update event log
        const logEl = document.getElementById('event-log');
        if (gameState.events) {
            logEl.innerHTML = gameState.events.map(e => `<div class="log-entry">${e}</div>`).join('');
            logEl.scrollTop = logEl.scrollHeight;
        }

        // Game over message
        if (gameState.phase === 'gameover') {
            let msg = '';
            if (gameState.result === 'white_win') msg = '白方获胜！';
            else if (gameState.result === 'black_win') msg = '黑方获胜！';
            else if (gameState.result === 'draw') msg = '和棋！';
            if (msg) {
                setTimeout(() => alert(msg), 100);
            }
        }
    }

    return { init };
})();

document.addEventListener('DOMContentLoaded', ChemissApp.init);
