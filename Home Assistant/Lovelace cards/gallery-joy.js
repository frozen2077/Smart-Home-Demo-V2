// import {generateList, generateMasonryGrid} from './app'

const fireEvent = (node, type, detail, options) => {
  options = options || {};
  detail = detail === null || detail === undefined ? {} : detail;
  const event = new Event(type, {
      bubbles: options.bubbles === undefined ? true : options.bubbles,
      cancelable: Boolean(options.cancelable),
      composed: options.composed === undefined ? true : options.composed,
  });
  event.detail = detail;
  node.dispatchEvent(event);
  return event;
};


class GalleryJoyCard extends HTMLElement {

  constructor() {
    super();
    this.attachShadow({ mode: 'open' });
    this._last_state;
  }

  setConfig(config) {
    if (!config.entity) {
      throw new Error('No Entity Configured');
    }
    this._config = deepClone(config);
    this._entity = this._config.entity;
    this._device = this._config.device;

    var root = this.shadowRoot;
    if (root.lastChild) root.removeChild(root.lastChild);
    var style = document.createElement('style');
    style.textContent = this._cssData();
    root.appendChild(style);
    this.hacard = document.createElement('ha-card');
    this.hacard.className = 'f-ha-card';
    this.hacard.innerHTML = this._htmlData();
    root.appendChild(this.hacard);   

    generateMasonryGrid(3, generateList(), root);

  }

  set hass(hass) {
    this._hass = hass;
    this._state = hass.states[this._entity]
    

    const forwardHaptic = hapticType => {
      fireEvent(window, "haptic", hapticType)
    }

    if (this._last_state != this._state)
      generateMasonryGrid(3, generateList(this._state.attributes["mobile"]), this.shadowRoot, hass, this._device);

    this._last_state = this._state;
    
  }


  _htmlData(){
    var html = `
        <div class="container">             
        </div>
    `;
    return html;
  }  

  _cssData(){
    var css = `
    ha-card {
      border-radius: var(--border-radius);
      box-shadow: var(--box-shadow);
      padding: 8px;
      backdrop-filter: var(--bg-filter);   
      background-color: var(--card-blur-color);    
    }

    *{
        margin:0;
        padding:0;
        box-sizing: border-box;
        color:white;
    }

    main{
        position: fixed;
        top:0;
        left: 0;
        width:100%;
        height: 100%;
        overflow: scroll;
    }
    .container{
        position: relative;
        width: 100%;
        display: flex;
        gap: 4px;
        padding: 0px;
        border-radius: calc(var(--border-radius) - 4px);
        height: 400px;
        overflow: scroll;        
    }
    .column{
        flex:1;
        display: flex;
        flex-direction: column;
        gap:4px;
    }
    .post{
        position: relative;
        overflow: visible;
        width:100%;
    }

    img{
        width: 100%;
        border-radius: 5px;
        height: 100%;
    }

    .overlay{
        position: absolute;
        top:0;
        left: 0;
        width:100%;
        height:100%;
        background:#161616;
        display: flex;
        align-items: center;
        justify-content: center;
        opacity:0;
        transition:0.5s;
        border-radius: 5px;
    }

    .post:hover .overlay{
        opacity: 0.5;
        cursor: pointer;
    }                      
    `;
    return css;
  }  
  
  getCardSize() {
    return 1;
  }    

} 


function deepClone(value) {
  if (!(!!value && typeof value == 'object')) {
    return value;
  }
  if (Object.prototype.toString.call(value) == '[object Date]') {
    return new Date(value.getTime());
  }
  if (Array.isArray(value)) {
    return value.map(deepClone);
  }
  var result = {};
  Object.keys(value).forEach(
    function(key) { result[key] = deepClone(value[key]); });
  return result;
}

function generateList(images) {
    const posts = [];
    const image_list = images ? images :
     [
      'https://i.pinimg.com/564x/73/bf/4b/73bf4b3dcd997370f172f5e5a4ddaa8f.jpg',
      'https://i.pinimg.com/474x/75/73/29/757329682e5c43c6f068f2cd236ab9f8.jpg',
      'https://i.pinimg.com/474x/3b/af/a4/3bafa44ae06f6ab124b6fd9dd63626fe.jpg',
      'https://i.pinimg.com/474x/b8/e9/23/b8e92321cdd7718ece99a55c2e130a42.jpg',
      'https://i.pinimg.com/474x/81/30/eb/8130eb00c3256ec7e001836b8671df5f.jpg',
      'https://i.pinimg.com/474x/23/b4/04/23b40489ada7f425975e7547b8787d21.jpg',
      'https://i.pinimg.com/474x/23/3e/87/233e87dc813f41d11e5be5128a881469.jpg',
      'https://i.pinimg.com/474x/db/73/29/db73295453e74be00132c983ca0a710f.jpg',
      'https://i.pinimg.com/474x/1b/39/da/1b39dafada167437638ac028d8ee94b7.jpg',
      'https://i.pinimg.com/474x/6d/93/b3/6d93b32c8fe8ba58b56cd5207f35bea4.jpg',
      'https://i.pinimg.com/474x/89/8a/36/898a360b7630c094420115da1f1547fb.jpg',
      'https://i.pinimg.com/474x/e8/3b/fa/e83bfa23df348d9cb78bfb0f788b480e.jpg',
      '/local/wallpaper/leaves6.jpg',
    ];
    
    let imageIndex = 0;
    
    for (let i = 1; i <= 80; i++) {
      let item = {
        id: i,
        title: `Post ${i}`,
        image: image_list[imageIndex],
      };
      posts.push(item);
      imageIndex++;
      if (imageIndex > image_list.length - 1) break;
    }
    return posts;
  }
  
  
function generateMasonryGrid(columns, posts, doc, hass, device) {
  
    const container = doc.querySelector('.container');
    container.innerHTML = '';
  
    //Store column arrays that contain relevant posts
    let columnWrappers = {};
  
    //Create column item array and  add this to column wrapper object
    for (let i = 0; i < columns; i++) {
      columnWrappers[`column${i}`] = [];
    }
    for (let i = 0; i < posts.length; i++) {
      const column = i % columns;
      columnWrappers[`column${column}`].push(posts[i]);
    }
    for (let i = 0; i < columns; i++) {
      let columnPosts = columnWrappers[`column${i}`];
      let column = document.createElement('div');
      column.classList.add('column');
      columnPosts.forEach((posts) => {
        let postDiv = document.createElement('div');
        postDiv.classList.add('post');
        let image = document.createElement('img');
        image.src = posts.image;
        let overlay = document.createElement('div');
        overlay.classList.add('overlay');
        let title = document.createElement('h3');
        title.innerText = posts.title;
  
        overlay.appendChild(title);
        overlay.addEventListener("click", () => {
          hass.callService('input_text', 'set_value', {
            entity_id: device,
            value: posts.image
          });
          fireEvent(window, "haptic", "success");
        });
        postDiv.append(image, overlay);
        column.appendChild(postDiv);
      });
      container.appendChild(column);
    }
  }

customElements.define("gallery-joy-card", GalleryJoyCard);

console.info("%c Gallery Joy CARD \n%c Version 1.0 ",
"color: orange; font-weight: bold; background: black", 
"color: white; font-weight: bold; background: dimgray");