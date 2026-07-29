/*
 * Copyright (c) 2022-2023 Shenzhen Kaihong Digital Industry Development Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
const vscode = require('vscode');
const fs = require('fs');
const path = require('path');
const { dirname } = require('path');
const { dir } = require('console');
const { getEventListeners } = require('stream');

function cutImgDict(context, msg) {
  let imgDir = context.extensionPath + '/images';
  let cutImgDict_ = msg.data.data;
  let whiteCutImg = fs.readFileSync(path.join(imgDir, cutImgDict_['whiteCut']));
  send('whiteCutImg', whiteCutImg);
  let circleImg = fs.readFileSync(path.join(imgDir, cutImgDict_['circleCut']));
  send('circleImg', circleImg);
  let cicleOpenImg = fs.readFileSync(path.join(imgDir, cutImgDict_['circleOpenCut']));
  send('cicleOpenImg', cicleOpenImg);
  let rectangleFocusImg = fs.readFileSync(path.join(imgDir, cutImgDict_['rectangleFocusCut']));
  send('rectangleFocusImg', rectangleFocusImg);
  let nodeIconImg = fs.readFileSync(path.join(imgDir, cutImgDict_['nodeIconCut']));
  send('nodeIconImg', nodeIconImg);
  let attrIconImg = fs.readFileSync(path.join(imgDir, cutImgDict_['attrIconCut']));
  send('attrIconImg', attrIconImg);
  let rootIconImg = fs.readFileSync(path.join(imgDir, cutImgDict_['rootIconCut']));
  send('rootIconImg', rootIconImg);
  let rootIconFocusImg = fs.readFileSync(path.join(imgDir, cutImgDict_['rootIconFocusCut']));
  send('rootIconFocusImg', rootIconFocusImg);
  let backgroundImg = fs.readFileSync(path.join(imgDir, cutImgDict_['backgroundCut']));
  send('backgroundImg', backgroundImg);
  let popItemFocusImg = fs.readFileSync(path.join(imgDir, cutImgDict_['popItemFocusCut']));
  send('popItemFocusImg', popItemFocusImg);

  let searchBgImg = fs.readFileSync(path.join(imgDir, cutImgDict_['searchBgCut']));
  send('searchBgImg', searchBgImg);
  let upCutImg = fs.readFileSync(path.join(imgDir, cutImgDict_['upCut']));
  send('upCutImg', upCutImg);
  let downCutImg = fs.readFileSync(path.join(imgDir, cutImgDict_['downCut']));
  send('downCut', downCutImg);
  let closeCutImg = fs.readFileSync(path.join(imgDir, cutImgDict_['closeCut']));
  send('closeCutImg', closeCutImg);
  let searchCutImg = fs.readFileSync(path.join(imgDir, cutImgDict_['searchCut']));
  send('searchCutImg', searchCutImg);
  let searchNoodRectImg = fs.readFileSync(path.join(imgDir, cutImgDict_['searchNoodRectImg']));
  send('searchNoodRectImg', searchNoodRectImg);
  let searchAttrRectImg = fs.readFileSync(path.join(imgDir, cutImgDict_['searchAttrRectImg']));
  send('searchAttrRectImg', searchAttrRectImg);
}

/**
 * @param {vscode.ExtensionContext} context
 */
function activate(context) {
  console.log('Congratulations, your extension "HCS View" is now active!');
  let disposable = vscode.commands.registerCommand('hcs_editor', function (uri) {
    vscode.window.showInformationMessage('Hello World from HCS View!');
    const color = new vscode.ThemeColor('badge.colorTheme');
    if (vscode.ccPanel != undefined) {
      vscode.ccPanel.dispose();
    } else {
      sendMsgFunc();
    }
    vscode.ccPanel = vscode.window.createWebviewPanel('ccnto','编辑' +
      path.basename(uri.fsPath), vscode.ViewColumn.Two, { enableScripts: true });
    vscode.ccPanel.webview.html = getWebviewContent(context, context.extensionUri);
    vscode.ccPanel.webview.onDidReceiveMessage((msg) => {
      switch (msg.type) {
        case 'inited':
          send('parse', uri.fsPath);
          break;
        case 'getfiledata':
          getFileData(msg);
          break;
        case 'writefile':
          fs.writeFileSync(msg.data.fn, msg.data.data);
          break;
        case 'selectchange':
          vscode.workspace.openTextDocument(msg.data).then((d) => {
            vscode.window.showTextDocument(d, vscode.ViewColumn.One);});
          break;
        case 'open_page':
          break;
        case 'error':
          vscode.window.showErrorMessage(msg.data);
          break;
        case 'cutImgDict':
          cutImgDict(context, msg);
          break;
        case 'reloadMenuBg':
          reloadMenuBgFunc(context, msg);
          break;
        default:
          vscode.window.showInformationMessage(msg.type);
          vscode.window.showInformationMessage(msg.data);
      }
    }, undefined, context.subscriptions);
    vscode.window.onDidChangeActiveColorTheme((colorTheme) => {
      send('colorThemeChanged', null);
    });
  });
  context.subscriptions.push(disposable);
}

function sendMsgFunc() {
  vscode.workspace.onDidSaveTextDocument((e) => {
    let tt2 = fs.readFileSync(e.uri.fsPath);
    let tt = new Int8Array(tt2);
    send('freshfiledata', {
      fn: e.uri.fsPath,
      data: tt,
    });
    send('parse', e.uri.fsPath);
  });
}

function reloadMenuBgFunc(context, msg) {
  let picDir = context.extensionPath + '/images';
  let menuBgPic = msg.data.data;
  let menuBgImg = fs.readFileSync(path.join(picDir, menuBgPic));
  send('backgroundImg', menuBgImg);
}

function getFileData(msg) {
  let tt2 = fs.readFileSync(msg.data);
  let tt = new Int8Array(tt2);
  send('filedata', {
    fn: msg.data, data: tt,
  });
}

function send(type, data) {
  vscode.ccPanel.webview.postMessage({
    type: type,
    data: data,
  });
}

function getWebviewContent(context, extpath) {
  let uri = vscode.Uri.joinPath(extpath, 'editor.html');
  let ret = fs.readFileSync(uri.fsPath, { encoding: 'utf8' });
  ret = getWebViewContent(context, '/editor.html');
  return ret;
}

function getWebViewContent(context, templatePath) {
  const resourcePath = path.join(context.extensionPath, templatePath);
  const dirPath = path.dirname(resourcePath);
  let html = fs.readFileSync(resourcePath, 'utf-8');
  html = html.replace(
    /(<link.+?href="|<script.+?src="|<iframe.+?src="|<img.+?src=")(.+?)"/g,
    (m, $1, $2) => {
      if ($2.indexOf('https://') < 0)
        return (
          $1 +
          vscode.Uri.file(path.resolve(dirPath, $2))
            .with({ scheme: 'vscode-resource' })
            .toString() +
          '"'
        );
      else return $1 + $2 + '"';
    }
  );
  return html;
}

function deactivate() {}

module.exports = {
  activate,
  deactivate,
};
