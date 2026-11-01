'use strict';
// Teste da rede do navegador (web/shell.html, criaRede) sem internet: duas
// paginas num servidor MQTT de mentira, com o relogio na mao. O servidor
// perde, repete e embaralha mensagens; uma pagina cai e volta; os marcos, a
// versao, as salas do site de teste e o convite.
//
//   node tests/rede.js [web/shell.html]
const fs = require('fs');
const path = require('path');

const caminho = process.argv[2] || path.join(__dirname, '..', 'web', 'shell.html');
const html = fs.readFileSync(caminho, 'utf8');
const bloco = html.match(/\/\* rede: inicio \*\/([\s\S]*?)\/\* rede: fim \*\//);
if (!bloco) { console.log('nao achei o bloco da rede em ' + caminho); process.exit(1); }
const criaRede = new Function(bloco[1] + '\nreturn criaRede;')();

// ---- O sorteio, o relogio e os timers de mentira ---------------------------
let rs = 12345;
function rnd() { rs ^= rs << 13; rs ^= rs >>> 17; rs ^= rs << 5; return (rs >>> 0) / 4294967296; }
let agora, fila, prox_id;
function agenda(f, ms, repete) {
  const id = prox_id++;
  fila.push({ quando: agora + ms, f, id, repete: repete ? ms : 0 });
  return id;
}
function cancela(id) { fila = fila.filter(t => t.id !== id); }
function anda(ms) {
  const fim = agora + ms;
  for (;;) {
    fila.sort((a, b) => a.quando - b.quando || a.id - b.id);
    const t = fila[0];
    if (!t || t.quando > fim) break;
    fila.shift();
    agora = t.quando;
    if (t.repete) fila.push({ quando: agora + t.repete, f: t.f, id: t.id, repete: t.repete });
    t.f();
  }
  agora = fim;
}

// ---- O servidor MQTT de mentira ---------------------------------------------
// Repassa cada mensagem a quem assina o topico, com atraso. Quem esta fora do
// ar perde o que chegou (sessao limpa); o que ele publicou fora do ar sai
// quando volta, como no mqtt.js.
let servidor;
function novo_mundo() {
  agora = 1e6; fila = []; prox_id = 1; rs = 12345;
  servidor = { clientes: new Set(), perde: null, repete: null, embaralha: false, recusa: null, topicos_usados: new Set() };
}
function conecta(url) {
  const c = { url, ouve: {}, topicos: new Set(), online: false, terminado: false, saida: [], chega: new Map() };
  c.on = (ev, f) => { (c.ouve[ev] = c.ouve[ev] || []).push(f); return c; };
  c.emite = (ev, ...a) => (c.ouve[ev] || []).forEach(f => f(...a));
  c.subscribe = (t, o, cb) => { c.topicos.add(t); servidor.topicos_usados.add(t); agenda(() => cb && cb(null), 5); };
  c.publish = (t, payload) => {
    if (c.terminado) return;
    if (!c.online) { c.saida.push([t, payload]); return; }
    entrega(c, t, payload);
  };
  c.end = () => { c.terminado = true; c.online = false; servidor.clientes.delete(c); };
  servidor.clientes.add(c);
  if (servidor.recusa && servidor.recusa(url)) return c;          // esse servidor nao responde
  agenda(() => { if (!c.terminado) { c.online = true; c.emite('connect'); } }, 15);
  return c;
}
function entrega(de, t, payload) {
  const msg = JSON.parse(payload);
  for (const para of servidor.clientes) {
    if (para.terminado || !para.topicos.has(t) || para.url !== de.url) continue;
    if (para !== de && servidor.perde && servidor.perde(msg)) continue;
    const vezes = para !== de && servidor.repete && servidor.repete(msg) ? 2 : 1;
    for (let i = 0; i < vezes; i++) {
      let q = agora + 20 + Math.floor(rnd() * 60);
      if (!servidor.embaralha) {                                      // a ordem de um remetente se mantem
        q = Math.max(q, para.chega.get(de) || 0);
        para.chega.set(de, q);
      }
      agenda(() => { if (para.online && !para.terminado) para.emite('message', t, Buffer.from(payload)); }, q - agora);
    }
  }
}
// A conexao de c cai por ms e volta (o mqtt.js reconecta e avisa 'connect').
function cai(c, ms) {
  c.online = false;
  if (ms === Infinity) return;
  agenda(() => {
    if (c.terminado) return;
    c.online = true;
    const s = c.saida; c.saida = [];
    s.forEach(([t, p]) => entrega(c, t, p));
    c.emite('connect');
  }, ms);
}

// ---- Uma pagina do jogo -----------------------------------------------------
function pagina(nome, op = {}) {
  const jogo = {
    comecou: null, teclas: [],
    _dado_online_comeca(s, eu) { this.comecou = { s, eu }; },
    _dado_rede_tecla(k) { this.teclas.push(k); },
  };
  const end = { origin: 'https://ericzv.github.io', pathname: op.caminho || '/dices/', search: op.busca || '' };
  const guardado = new Map(Object.entries(op.guarda || {}));
  const pag = { nome, jogo, end, clientes: [], copiados: [], mandadas: [] };
  pag.rede = criaRede({
    mqtt: { connect: url => { const c = conecta(url); pag.clientes.push(c); return c; } },
    jogo: () => jogo,
    endereco: end,
    troca_endereco: url => { const u = new URL(url); end.pathname = u.pathname; end.search = u.search; },
    guarda: { getItem: k => guardado.has(k) ? guardado.get(k) : null,
              setItem: (k, v) => guardado.set(k, String(v)), removeItem: k => guardado.delete(k) },
    versao: op.versao || '?v=abc1234',
    agora: () => agora,
    aleatorio: rnd,
    setTimeout: (f, ms) => agenda(f, ms, false), clearTimeout: cancela,
    setInterval: (f, ms) => agenda(f, ms, true), clearInterval: cancela,
    compartilha: null,
    copia: texto => { pag.copiados.push(texto); return op.copia_falha ? Promise.reject(new Error('nao')) : Promise.resolve(); },
  });
  pag.cliente = () => pag.clientes[pag.clientes.length - 1];
  pag.manda = k => { pag.mandadas.push(k); pag.rede.envia(k); };
  return pag;
}
const PARADA = 0, ESPERANDO = 2, JOGANDO = 3, ERRO = 4;

// ---- Os casos ---------------------------------------------------------------
let falhas = 0, casos = 0;
function confere(ok, oque) { if (!ok) { falhas++; console.log('  FALHOU: ' + oque); } }
async function caso(nome, f) {
  casos++;
  novo_mundo();
  const antes = falhas;
  try { await f(); } catch (e) { falhas++; console.log('  ERRO: ' + (e && e.stack || e)); }
  console.log((falhas === antes ? 'ok     ' : 'FALHOU ') + nome);
}
// Cria a sala em A e entra com B; devolve as duas paginas ja jogando.
function partida(opA = {}, opB = {}) {
  const A = pagina('A', opA);
  A.rede.cria(); anda(200);
  const B = pagina('B', opB);
  B.rede.entra(A.rede.codigo); anda(600);
  return [A, B];
}
const iguais = (a, b) => a.length === b.length && a.every((x, i) => x === b[i]);

(async () => {
  await caso('sala, convite no endereco e comeco igual nos dois', () => {
    const A = pagina('A');
    A.rede.cria(); anda(200);
    const cod = A.rede.codigo;
    confere(A.rede.estado === ESPERANDO, 'A esperando');
    confere(/^[A-HJ-NP-Z2-9]{4}$/.test(cod), 'codigo de 4 letras: ' + cod);
    confere(A.rede.convite === 'https://ericzv.github.io/dices/?sala=' + cod, 'link: ' + A.rede.convite);
    confere(A.end.search === '?sala=' + cod, 'o endereco de A vira o convite');
    const B = pagina('B', { busca: '?sala=' + cod.toLowerCase() });
    const pend = B.rede.convite_pendente();
    confere(pend === cod, 'quem abre o link reconhece o convite: ' + pend);
    B.rede.entra(pend); anda(600);
    confere(A.rede.estado === JOGANDO && B.rede.estado === JOGANDO, 'os dois jogando');
    confere(A.jogo.comecou && B.jogo.comecou && A.jogo.comecou.s === B.jogo.comecou.s, 'a mesma semente');
    confere(A.jogo.comecou.eu === 0 && B.jogo.comecou.eu === 1, 'quem criou e 0, quem entrou e 1');
    confere(A.end.search === '' && B.end.search === '', 'o convite sai do endereco quando a partida comeca');
  });

  await caso('teclas com perdas, repetidas e fora de ordem chegam todas, em ordem', () => {
    const [A, B] = partida();
    servidor.perde = msg => msg.t !== 'oi' && rnd() < 0.2;
    servidor.repete = () => rnd() < 0.1;
    servidor.embaralha = true;
    for (let i = 0; i < 300; i++) {
      (i % 3 ? A : B).manda(0x100 + (i * 7) % 300);
      anda(Math.floor(rnd() * 120));
    }
    anda(30000);
    confere(iguais(B.jogo.teclas, A.mandadas), 'B recebeu ' + B.jogo.teclas.length + ' de ' + A.mandadas.length + ', na ordem');
    confere(iguais(A.jogo.teclas, B.mandadas), 'A recebeu ' + A.jogo.teclas.length + ' de ' + B.mandadas.length + ', na ordem');
    confere(A.rede.estado === JOGANDO && B.rede.estado === JOGANDO, 'a partida segue: ' + A.rede.erro + B.rede.erro);
  });

  await caso('o "comeca" se perde e as primeiras teclas chegam antes da partida: nada se perde', () => {
    const A = pagina('A');
    A.rede.cria(); anda(200);
    let perdeu = 0;
    servidor.perde = msg => msg.t === 'comeca' && !perdeu++;
    const B = pagina('B');
    B.rede.entra(A.rede.codigo); anda(150);
    confere(A.rede.estado === JOGANDO && B.rede.estado !== JOGANDO, 'A ja comecou, B ainda nao');
    for (let i = 0; i < 5; i++) { A.manda(40 + i); anda(100); }       // B ainda nao esta jogando: ignora
    anda(8000);
    confere(B.rede.estado === JOGANDO, 'B comecou no segundo "comeca"');
    confere(iguais(B.jogo.teclas, A.mandadas), 'B recebeu as teclas de antes: ' + B.jogo.teclas.length);
  });

  await caso('a conexao cai por 6 s e volta: nada se perde', () => {
    const [A, B] = partida();
    cai(B.cliente(), 6000);
    for (let i = 0; i < 20; i++) { A.manda(i); B.manda(100 + i); anda(200); }
    anda(15000);
    confere(iguais(B.jogo.teclas, A.mandadas), 'B recebeu as ' + B.jogo.teclas.length + ' de A');
    confere(iguais(A.jogo.teclas, B.mandadas), 'A recebeu as ' + A.jogo.teclas.length + ' de B');
    confere(A.rede.estado === JOGANDO && B.rede.estado === JOGANDO, 'a partida segue');
  });

  await caso('marcos iguais, cada lado no seu tempo: a partida segue', () => {
    const [A, B] = partida();
    for (let c = 1; c <= 40; c++) {
      A.rede.marco(c, (c * 2654435761) >>> 0);
      anda(300);
      if (c > 3) B.rede.marco(c - 3, ((c - 3) * 2654435761) >>> 0);       // B vem tres marcos atras
    }
    for (let c = 38; c <= 40; c++) B.rede.marco(c, (c * 2654435761) >>> 0);
    anda(10000);
    confere(A.rede.estado === JOGANDO && B.rede.estado === JOGANDO, 'nao parou: ' + A.rede.erro + B.rede.erro);
  });

  await caso('um marco diferente para a partida nos dois lados, com o aviso', () => {
    const [A, B] = partida();
    for (let c = 1; c <= 5; c++) { A.rede.marco(c, c); B.rede.marco(c, c); anda(500); }
    for (let c = 6; c <= 12; c++) { A.rede.marco(c, c); anda(400); }      // A vai na frente
    for (let c = 6; c <= 12; c++) { B.rede.marco(c, c === 9 ? 999 : c); anda(400); }
    anda(8000);
    confere(A.rede.estado === ERRO && B.rede.estado === ERRO, 'os dois param: ' + A.rede.estado + '/' + B.rede.estado);
    confere(/diferentes/.test(A.rede.erro) && /diferentes/.test(B.rede.erro), 'o aviso: ' + A.rede.erro + ' | ' + B.rede.erro);
  });

  await caso('versoes diferentes nao jogam juntas', () => {
    const A = pagina('A', { versao: '?v=1111111' });
    A.rede.cria(); anda(200);
    const B = pagina('B', { versao: '?v=2222222' });
    B.rede.entra(A.rede.codigo); anda(2000);
    confere(!A.jogo.comecou && !B.jogo.comecou, 'a partida nao comeca');
    confere(A.rede.estado === ERRO && /Versões/.test(A.rede.erro), 'quem criou: ' + A.rede.erro);
    confere(B.rede.estado === ERRO && /Versões/.test(B.rede.erro), 'quem entrou: ' + B.rede.erro);
  });

  await caso('o site de teste tem salas separadas das do site principal', () => {
    const A = pagina('A', { caminho: '/dices/teste/' });
    A.rede.cria(); anda(200);
    confere(A.rede.topico().indexOf('dadoemcasa/v2/teste/') === 0, 'topico de teste: ' + A.rede.topico());
    confere(A.rede.convite.indexOf('/dices/teste/?sala=') > 0, 'o convite leva ao site de teste');
    const B = pagina('B', { caminho: '/dices/' });
    B.rede.entra(A.rede.codigo); anda(20000);
    confere(B.rede.estado === ERRO && /Não achei/.test(B.rede.erro), 'do site principal nao acha: ' + B.rede.erro);
    confere(A.rede.estado === ESPERANDO, 'a sala de teste segue esperando');
    const C = pagina('C', { caminho: '/dices/teste/' });
    C.rede.entra(A.rede.codigo); anda(600);
    confere(A.rede.estado === JOGANDO && C.rede.estado === JOGANDO, 'do site de teste entra');
  });

  await caso('a aba que criou a sala recarrega: nao entra na propria sala', () => {
    const P = pagina('P', { busca: '?sala=ABCD', guarda: { dadoemcasa_sala_minha: 'ABCD' } });
    confere(P.rede.convite_pendente() === '', 'nao e convite');
    confere(P.end.search === '', 'o endereco volta ao de sempre');
    const Q = pagina('Q', { busca: '?sala=AB1D&servidor=wss://local' });
    confere(Q.rede.convite_pendente() === '', 'codigo invalido no link nao vale');
    confere(Q.end.search === '?servidor=wss%3A%2F%2Flocal', 'o resto do endereco fica: ' + Q.end.search);
  });

  await caso('o rival some: a partida para depois de 25 s sem noticia', () => {
    const [A, B] = partida();
    cai(A.cliente(), Infinity);
    anda(20000);
    confere(B.rede.estado === JOGANDO, 'ainda espera aos 20 s');
    anda(10000);
    confere(B.rede.estado === ERRO && /parou de responder/.test(B.rede.erro), 'aviso: ' + B.rede.erro);
  });

  await caso('o rival sai: o outro ve o aviso na hora', () => {
    const [A, B] = partida();
    A.rede.sai(); anda(500);
    confere(A.rede.estado === PARADA, 'quem saiu volta ao comeco');
    confere(B.rede.estado === ERRO && /saiu/.test(B.rede.erro), 'aviso: ' + B.rede.erro);
  });

  await caso('copiar o convite: uma vez so, e o aviso quando nao da', async () => {
    const A = pagina('A');
    A.rede.cria(); anda(200);
    A.rede.copia(); A.rede.copia();
    await Promise.resolve(); await Promise.resolve();
    confere(A.copiados.length === 1 && A.copiados[0] === A.rede.convite, 'copiou o link uma vez: ' + A.copiados.length);
    confere(A.rede.copiado === 1, 'avisou que copiou');
    const F = pagina('F', { copia_falha: true });
    F.rede.cria(); anda(200);
    F.rede.copia();
    await Promise.resolve(); await Promise.resolve(); await Promise.resolve();
    confere(F.rede.copiado === -1, 'avisou que nao deu: ' + F.rede.copiado);
  });

  await caso('servidor fora do ar: a sala abre no proximo, e quem entra vai junto', () => {
    servidor.recusa = url => url.indexOf('hivemq') >= 0;
    const A = pagina('A');
    A.rede.cria(); anda(12000);
    confere(A.rede.estado === ESPERANDO && A.cliente().url.indexOf('emqx') >= 0, 'abriu no segundo: ' + A.cliente().url);
    const B = pagina('B');
    B.rede.entra(A.rede.codigo); anda(600);
    confere(B.cliente().url === A.cliente().url, 'quem entra vai ao mesmo servidor');
    confere(A.rede.estado === JOGANDO && B.rede.estado === JOGANDO, 'os dois jogando');
  });

  console.log(falhas ? '\n' + falhas + ' falha(s) em ' + casos + ' casos' : '\n' + casos + ' casos ok');
  process.exit(falhas ? 1 : 0);
})();
