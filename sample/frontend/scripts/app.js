const THEME_STORAGE_KEY = 'kutie-sample-theme';

function logEvent(message) {
  const target = document.getElementById('event-log');
  const line = `[${new Date().toLocaleTimeString()}] ${message}\n`;
  target.textContent = line + target.textContent;
}

function setOutput(id, value) {
  document.getElementById(id).textContent = typeof value === 'string' ? value : JSON.stringify(value, null, 2);
}

function setTitlebarMode(custom) {
  document.getElementById('titlebar').classList.toggle('hidden', !custom);
  document.getElementById('native-header').classList.toggle('hidden', custom);
  document.getElementById('custom-header').classList.toggle('hidden', !custom);
}

function setTheme(theme) {
  const next = theme === 'light' ? 'light' : 'dark';
  document.documentElement.setAttribute('data-theme', next);
  try {
    localStorage.setItem(THEME_STORAGE_KEY, next);
  } catch {
    // Ignore storage failures in restricted contexts.
  }
  logEvent(`theme=${next}`);
}

function initTheme() {
  let saved = 'dark';
  try {
    saved = localStorage.getItem(THEME_STORAGE_KEY) || 'dark';
  } catch {
    saved = 'dark';
  }
  setTheme(saved === 'light' ? 'light' : 'dark');
}

async function init() {
  initTheme();

  if (!window.kutie) {
    setOutput('greet-output', 'Kutie runtime bridge is not available.');
    return;
  }

  setTitlebarMode(false);

  kutie.on('heartbeat', (data) => {
    logEvent(`heartbeat tick=${data.tick}`);
  });

  document.getElementById('btn-greet').addEventListener('click', async () => {
    const name = document.getElementById('name-input').value.trim() || 'World';
    const result = await kutie.call('greet', { name });
    setOutput('greet-output', result);
  });

  document.getElementById('btn-minimize').addEventListener('click', () => kutie.call('shell.minimize'));
  document.getElementById('btn-maximize').addEventListener('click', () => kutie.call('shell.maximize'));
  document.getElementById('btn-restore').addEventListener('click', () => kutie.call('shell.restore'));

  document.getElementById('btn-native-titlebar').addEventListener('click', async () => {
    await kutie.call('shell.set_decorations', { decorations: true });
    setTitlebarMode(false);
  });

  document.getElementById('btn-custom-titlebar').addEventListener('click', async () => {
    await kutie.call('shell.set_decorations', { decorations: false });
    setTitlebarMode(true);
  });

  document.getElementById('btn-theme-dark').addEventListener('click', () => setTheme('dark'));
  document.getElementById('btn-theme-light').addEventListener('click', () => setTheme('light'));

  document.querySelectorAll('[data-action]').forEach((button) => {
    button.addEventListener('click', () => {
      const action = button.getAttribute('data-action');
      if (action === 'minimize') kutie.call('shell.minimize');
      if (action === 'maximize') kutie.call('shell.toggle_maximize');
      if (action === 'close') kutie.call('shell.close');
    });
  });

  document.getElementById('btn-open-file').addEventListener('click', async () => {
    setOutput('system-output', await kutie.call('dialog.open', { title: 'Open File' }));
  });

  document.getElementById('btn-save-file').addEventListener('click', async () => {
    setOutput('system-output', await kutie.call('dialog.save', { title: 'Save File', default_name: 'notes.txt' }));
  });

  document.getElementById('btn-pick-folder').addEventListener('click', async () => {
    setOutput('system-output', await kutie.call('dialog.folder', { title: 'Pick Folder' }));
  });

  document.getElementById('btn-clipboard-read').addEventListener('click', async () => {
    setOutput('system-output', await kutie.call('clipboard.read'));
  });

  document.getElementById('btn-clipboard-write').addEventListener('click', async () => {
    setOutput('system-output', await kutie.call('clipboard.write', { text: 'Hello from Kutie' }));
  });

  setOutput('system-output', await kutie.call('system.info'));
}

window.addEventListener('DOMContentLoaded', init);
