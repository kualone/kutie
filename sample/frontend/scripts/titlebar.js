function bindTitlebarControls() {
  if (!window.kutie) {
    return;
  }
  const current = () => kutie.BrowserWindow.getCurrent();
  document.querySelectorAll('[data-action]').forEach((button) => {
    button.addEventListener('click', () => {
      const action = button.getAttribute('data-action');
      if (action === 'minimize') current().minimize();
      if (action === 'maximize') current().toggleMaximize();
      if (action === 'close') current().close();
    });
  });
}

window.addEventListener('DOMContentLoaded', bindTitlebarControls);
