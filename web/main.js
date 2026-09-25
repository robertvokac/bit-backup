async function copyText(value) {
  if (navigator.clipboard && window.isSecureContext) {
    await navigator.clipboard.writeText(value);
    return;
  }

  const helper = document.createElement('textarea');
  helper.value = value;
  helper.style.position = 'fixed';
  helper.style.opacity = '0';
  document.body.append(helper);
  helper.select();
  const copied = document.execCommand('copy');
  helper.remove();
  if (!copied) throw new Error('Copy failed');
}

document.querySelectorAll('[data-copy-target]').forEach((button) => {
  const originalLabel = button.textContent;
  let resetTimer;

  button.addEventListener('click', async () => {
    const target = document.getElementById(button.dataset.copyTarget);
    if (!target) return;

    try {
      await copyText(target.textContent.trim());
      button.textContent = 'Copied!';
    } catch {
      button.textContent = 'Copy failed';
    }

    clearTimeout(resetTimer);
    resetTimer = setTimeout(() => { button.textContent = originalLabel; }, 2200);
  });
});
