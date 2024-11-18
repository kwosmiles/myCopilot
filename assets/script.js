//此js文件会在网页导航完成之后注入到webview中
//此处的环境是浏览器环境
//如果需要调试，请在config.hpp打开浏览器开发者模式

const injectedScript = {
    //测试函数
    //启用这个函数将在webview中
    //弹出信息，以便开发者观察js代码注入时机
    testAlert:function(){
        alert("Hello Cpilot!");
    },

    //给网页的滚动条添加样式
    //让滚动条变小变透明
    //给body背景添加颜色，避免主要内容未渲染时闪白屏
    customizeScrollbar:function(){
        const style = document.createElement('style');
        style.textContent = `
            body {
                background-color:rgb(250,240,231);
            }

            ::-webkit-scrollbar {
            width: 5px;
            height: 8px;
            }

            ::-webkit-scrollbar-track {
            background: transparent;
            border-radius: 10px;
            }

            ::-webkit-scrollbar-thumb {
            background-color: rgba(0, 0, 0, 0.2);
            border-radius: 5px;
            border: 1px solid transparent;
            background-clip: content-box;
            }
        `;
        document.head.appendChild(style);  
    }
}


//上方是函数的定义和实现
//下方是函数的开始
//不想使用的功能直接注释掉
//===============================

//injectedScript.testAlert();
injectedScript.customizeScrollbar();