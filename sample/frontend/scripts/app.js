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

async function enableCustomTitlebar() {
  if (!window.kutie) {
    return;
  }
  await kutie.BrowserWindow.getCurrent().setFrame(false);
  setTitlebarMode(true);
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

  await enableCustomTitlebar();

  kutie.on('heartbeat', (data) => {
    logEvent(`heartbeat tick=${data.tick}`);
  });

  document.getElementById('btn-greet').addEventListener('click', async () => {
    const name = document.getElementById('name-input').value.trim() || 'World';
    const result = await kutie.call('greet', { name });
    setOutput('greet-output', result);
  });

  const current = () => kutie.BrowserWindow.getCurrent();

  document.getElementById('btn-minimize').addEventListener('click', () => current().minimize());
  document.getElementById('btn-maximize').addEventListener('click', () => current().maximize());
  document.getElementById('btn-restore').addEventListener('click', () => current().restore());

  document.getElementById('btn-native-titlebar').addEventListener('click', async () => {
    await current().setFrame(true);
    setTitlebarMode(false);
  });

  document.getElementById('btn-custom-titlebar').addEventListener('click', () => {
    enableCustomTitlebar();
  });

  document.getElementById('btn-theme-dark').addEventListener('click', () => setTheme('dark'));
  document.getElementById('btn-theme-light').addEventListener('click', () => setTheme('light'));

  document.getElementById('btn-open-modal').addEventListener('click', async () => {
    const parent = current();
    await kutie.BrowserWindow.create({
      title: 'Modal Dialog',
      url: 'https://assets.kutie/dialog.html',
      width: 480,
      height: 320,
      parent_id: parent.id,
      modal: true,
      frame: false,
      center: true,
    });
    logEvent('modal opened');
  });

  document.getElementById('btn-open-child').addEventListener('click', async () => {
    const parent = current();
    await kutie.BrowserWindow.create({
      title: 'Child Window',
      url: 'https://assets.kutie/child-window.html',
      width: 460,
      height: 300,
      parent_id: parent.id,
      modal: false,
      frame: false,
      show_in_taskbar: false,
      center: true,
    });
    logEvent('child window opened (no taskbar icon)');
  });

  document.getElementById('btn-open-standalone').addEventListener('click', async () => {
    await kutie.BrowserWindow.create({
      title: 'Independent Window',
      url: 'https://assets.kutie/standalone-window.html',
      width: 520,
      height: 340,
      frame: false,
      center: true,
    });
    logEvent('independent window opened (own taskbar icon)');
  });

  document.querySelectorAll('[data-action]').forEach((button) => {
    button.addEventListener('click', () => {
      const action = button.getAttribute('data-action');
      if (action === 'minimize') current().minimize();
      if (action === 'maximize') current().toggleMaximize();
      if (action === 'close') current().close();
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
