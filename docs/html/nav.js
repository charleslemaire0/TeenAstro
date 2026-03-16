// TeenAstro Docs — shared navigation logic

function initNav() {
  const sections = document.querySelectorAll('section[id]');
  const links = document.querySelectorAll('#nav a[href^="#"]');
  if (!sections.length || !links.length) return;

  function update() {
    let current = '';
    sections.forEach(s => { if (scrollY >= s.offsetTop - 80) current = s.id; });
    links.forEach(a => a.classList.toggle('active', a.getAttribute('href') === '#' + current));
  }
  window.addEventListener('scroll', update, { passive: true });
  update();
}

function toggleSidebar() {
  document.getElementById('sidebar').classList.toggle('open');
}

document.addEventListener('DOMContentLoaded', () => {
  initNav();
  const c = document.getElementById('content');
  if (c) c.addEventListener('click', () => document.getElementById('sidebar').classList.remove('open'));
});
