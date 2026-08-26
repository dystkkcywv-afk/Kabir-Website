const projects={
  snake:{index:"01 / PROJECT FIELD NOTE",kind:"GAME SYSTEMS · C",title:"Snake Game Engine",summary:"A modular grid-based engine with movement, collisions, portals, typed consumables, directional passages, explosions and game-state management.",scope:"Individual university project",tools:"C · structs · arrays · pointers",next:"Separate more systems into reusable modules and add automated gameplay tests.",built:"I structured the growing game around focused functions and state containers. Each new mechanic had to integrate with movement, scoring and the board without breaking earlier behaviour.",learned:"I learned that managing state is often harder than writing the individual feature. Clear responsibilities and careful debugging became more important as the system expanded.",source:"cs_snake.c",code:`if (consumable == APPLE_REVERSE) {\n    board[*row][*col].entity = BODY_SEGMENT;\n    apply_reverse_apple(row, col, body_r, body_c, body_len);\n    *points += 10;\n}`},
  runtime:{index:"02 / PROJECT FIELD NOTE",kind:"SYSTEMS SOFTWARE · C · POSIX",title:"Standard I/O Runtime",summary:"A lightweight standard I/O implementation built directly on POSIX system calls, including stream state, Unicode handling and process creation.",scope:"Individual systems project",tools:"C · POSIX · Linux · UTF-8",next:"Strengthen edge-case testing and benchmark operations against familiar standard-library behaviour.",built:"I wrapped file descriptors in a stream interface, tracked EOF and error states, decoded UTF-8 input and implemented PATH resolution with fork and exec for process creation.",learned:"Working below familiar library functions gave me a clearer view of resource ownership, operating-system boundaries and how small edge cases shape reliable APIs.",source:"cs1521_stdio.c",code:`int fd = open(pathname, flags, 0644);\nif (fd < 0) return NULL;\n\ncs1521_FILE *stream = malloc(sizeof *stream);\nstream->fd = fd;\nstream->eof = 0;\nstream->err = 0;`},
  team:{index:"03 / PROJECT FIELD NOTE",kind:"INTRODUCTORY TEAM EXPOSURE",title:"Team Simulation Platform",summary:"Three weeks of introductory contribution to a shared JavaScript project before changing courses.",scope:"Short group-based course exposure",tools:"JavaScript · GitLab · basic testing",next:"Revisit collaborative development through a complete personal or hackathon project.",built:"I practised JavaScript functions, conditions, input validation and basic tests while learning branches, commits and the responsibilities of working in a shared repository.",learned:"The short experience introduced me to the discipline of making small understandable changes and coordinating them with other people. I present it as foundation-level exposure rather than a completed project.",code:`function isValidInput(value) {\n  if (value === undefined || value === null) {\n    return false;\n  }\n  return String(value).trim().length > 0;\n}`}
};

const cards=[...document.querySelectorAll(".card")];
const filters=[...document.querySelectorAll(".filter")];
const reduced=matchMedia("(prefers-reduced-motion: reduce)");
const motion=!reduced.matches;

function animateCards(){cards.filter(card=>!card.classList.contains("is-hidden")).forEach((card,index)=>{card.style.setProperty("--index",index);card.classList.remove("is-entering");void card.offsetWidth;card.classList.add("is-entering")})}
function applyFilter(group,button){const change=()=>{cards.forEach(card=>card.classList.toggle("is-hidden",group!=="all"&&card.dataset.group!==group));filters.forEach(item=>item.classList.toggle("active",item===button));animateCards();window.scrollTo({top:0,behavior:motion?"smooth":"auto"})};if(document.startViewTransition&&motion)document.startViewTransition(change);else change()}
filters.forEach(button=>button.addEventListener("click",()=>applyFilter(button.dataset.filter,button)));

const caseDialog=document.querySelector("#case-modal");
function openCase(key){const project=projects[key];if(!project)return;["index","kind","title","summary","scope","tools","next","built","learned","code"].forEach(field=>document.querySelector(`#modal-${field}`).textContent=project[field]);const source=document.querySelector("#modal-source");source.hidden=!project.source;if(project.source)source.href=project.source;caseDialog.showModal();document.body.style.overflow="hidden"}
document.querySelectorAll("[data-panel]").forEach(button=>button.addEventListener("click",()=>{const panel=button.dataset.panel;if(projects[panel])openCase(panel);if(panel==="about"){document.querySelector("#profile-modal").showModal();document.body.style.overflow="hidden"}if(panel==="gallery"){document.querySelector("#gallery-modal").showModal();document.body.style.overflow="hidden"}}));
function closeDialog(dialog){dialog.close();document.body.style.overflow=""}
document.querySelectorAll(".modal-close").forEach(button=>button.addEventListener("click",()=>closeDialog(caseDialog)));
document.querySelectorAll(".profile-close").forEach(button=>button.addEventListener("click",()=>closeDialog(document.querySelector("#profile-modal"))));
document.querySelectorAll(".gallery-close").forEach(button=>button.addEventListener("click",()=>closeDialog(document.querySelector("#gallery-modal"))));
document.querySelectorAll("dialog").forEach(dialog=>dialog.addEventListener("click",event=>{if(event.target===dialog)closeDialog(dialog)}));

const glow=document.querySelector(".cursor-glow");
window.addEventListener("pointermove",event=>{glow.style.left=`${event.clientX}px`;glow.style.top=`${event.clientY}px`},{passive:true});
document.querySelectorAll(".card").forEach(card=>{card.addEventListener("pointermove",event=>{if(!motion||innerWidth<700)return;const box=card.getBoundingClientRect();const x=(event.clientX-box.left)/box.width-.5;const y=(event.clientY-box.top)/box.height-.5;card.style.transform=`perspective(900px) rotateX(${-y*2.3}deg) rotateY(${x*2.3}deg) translateY(-2px)`});card.addEventListener("pointerleave",()=>card.style.transform="")});
document.querySelectorAll(".magnetic").forEach(element=>{element.addEventListener("pointermove",event=>{if(!motion)return;const box=element.getBoundingClientRect();element.style.transform=`translate(${(event.clientX-box.left-box.width/2)*.12}px,${(event.clientY-box.top-box.height/2)*.12}px)`});element.addEventListener("pointerleave",()=>element.style.transform="")});

const themeButton=document.querySelector(".theme-toggle");
const preferredTheme=localStorage.getItem("kabir-theme")||"light";
function setTheme(theme){const dark=theme==="dark";document.documentElement.dataset.theme=theme;themeButton.setAttribute("aria-pressed",String(dark));themeButton.setAttribute("aria-label",dark?"Switch to light mode":"Switch to dark mode");themeButton.innerHTML=dark?"<span>☀</span> Light mode":"<span>◐</span> Dark mode";localStorage.setItem("kabir-theme",theme)}
setTheme(preferredTheme);
themeButton.addEventListener("click",()=>setTheme(document.documentElement.dataset.theme==="dark"?"light":"dark"));
const header=document.querySelector(".site-head");const topButton=document.querySelector(".to-top");let lastY=0;
addEventListener("scroll",()=>{header.classList.toggle("compact",scrollY>90);topButton.classList.toggle("visible",scrollY>500);if(scrollY>lastY&&scrollY>300)header.style.transform="translateY(-18px)";else header.style.transform="";lastY=scrollY},{passive:true});
topButton.addEventListener("click",()=>scrollTo({top:0,behavior:motion?"smooth":"auto"}));
addEventListener("keydown",event=>{if(event.key==="Escape")document.body.style.overflow=""});
animateCards();
