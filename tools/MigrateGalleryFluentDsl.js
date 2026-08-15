const fs=require('fs');
const pages='CUI.Gallery/src/pages/BasicInput/Pages.h'; let h=fs.readFileSync(pages,'utf8'); const needle='namespace Gallery {\r\n'; if(!h.includes('using CUI::DSL::Fluent::Button;')){ h=h.replace(/namespace Gallery \{\r?\n/, 'namespace Gallery {\n\nusing CUI::DSL::Fluent::Button;\n'); } fs.writeFileSync(pages,h);
const path=require('path'); const root=path.resolve('CUI.Gallery/src');
function walk(dir){return fs.readdirSync(dir,{withFileTypes:true}).flatMap(e=>{const f=path.join(dir,e.name); return e.isDirectory()?walk(f):/\.(cpp|h)$/.test(e.name)?[f]:[];});}
function findMatching(text,start,open,close){let depth=0; let quote=null,escape=false; for(let i=start;i<text.length;i++){const c=text[i]; if(quote){if(escape) escape=false; else if(c==='\\') escape=true; else if(c===quote) quote=null; continue;} if(c==='"'||c==="'"){quote=c;continue;} if(c===open)depth++; else if(c===close && --depth===0)return i;} return -1;}
let changed=[]; let report=[];
for(const file of walk(root)){
 let text=fs.readFileSync(file,'utf8'), original=text;

 // Migrate controls that already have a semantic DSL factory.
 const factoryMap = {
   TextBox: 'TextField', TextBlock: 'Text', CheckBox: 'CheckboxTile', RadioButton: 'RadioButtonTile',
   ToggleButton: 'ToggleButtonWidget', DropDownButton: 'DropDownButtonWidget', SplitButton: 'SplitButtonWidget',
   HyperlinkButton: 'HyperlinkButtonWidget', Slider: 'SliderWidget', RangeSlider: 'RangeSliderWidget',
   RatingControl: 'RatingWidget', DatePicker: 'DatePickerWidget', TimePicker: 'TimePickerWidget',
   ColorPicker: 'ColorPickerWidget', SegmentedControl: 'SegmentedWidget', NumberBox: 'NumberBoxWidget',
   PasswordBox: 'PasswordBoxWidget', Expander: 'ExpanderWidget', ListView: 'ListViewWidget',
   ContentDialog: 'ContentDialogWidget', TeachingTip: 'TeachingTipWidget', Flyout: 'FlyoutWidget', ComboBox: 'ComboBoxWidget', ListBox: 'ListBoxWidget', ToggleSwitch: 'ToggleSwitchWidget', TreeView: 'TreeViewWidget', Canvas: 'CanvasWidget',
   CanvasControl: 'CanvasControlWidget', Grid: 'GridWidget', WrapPanel: 'WrapPanelWidget', DockPanel: 'DockPanelWidget',
   UniformGrid: 'UniformGridWidget', Splitter: 'SplitterWidget', Image: 'ImageWidget',
   ProgressBar: 'ProgressBarWidget', ProgressRing: 'ProgressRingWidget', AutoSuggestBox: 'AutoSuggestBoxWidget',
   StatusBar: 'StatusBarWidget', CommandBar: 'CommandBarWidget', MenuBar: 'MenuBarWidget',
   BreadcrumbBar: 'BreadcrumbBarWidget', PagingControl: 'PagingControlWidget', MarkdownView: 'MarkdownViewWidget',
   LogView: 'LogViewWidget', InfoBar: 'InfoBarWidget', FilePicker: 'FilePickerWidget', FolderPicker: 'FolderPickerWidget'
 };
 for (const [type, factory] of Object.entries(factoryMap)) {
   text = text.replace(new RegExp('\\bMake<\\s*' + type + '\\s*>\\s*\\(([^;\\n]*)\\)', 'g'), factory + '($1)');
 }
 // Button factory migration. Qualify type-only occurrences first, then factory calls.
 text=text.replace(/\bMake<\s*Button\s*>\s*\(/g,'Button(').replace(/std::make_shared<\s*Button\s*>\s*\(/g,'Button(');
 text=text.replace(/std::shared_ptr<\s*Button\s*>/g,'std::shared_ptr<CUI::Button>');
 // Migrate Row/Column(...).Children({...}).Build() with balanced delimiters.
 let out='', cursor=0, i=0;
 while(i<text.length){
   const match=text.slice(i).match(/\b(Row|Column)\s*\(/); if(!match){out+=text.slice(i);break;}
   const start=i+match.index, name=match[1], open=start+text.slice(start).indexOf('('), close=findMatching(text,open,'(',')');
   if(close<0){out+=text.slice(i);break;}
   const after=text.slice(close+1).match(/^\.Children\s*\(\s*\{/); if(!after){out+=text.slice(i,close+1);i=close+1;continue;}
   const brace=close+1+after[0].lastIndexOf('{'), braceClose=findMatching(text,brace,'{','}');
   if(braceClose<0){out+=text.slice(i);break;}
   const suffix=text.slice(braceClose+1).match(/^\s*\)\s*\.Build\s*\(\s*\)/);
   if(!suffix){out+=text.slice(i,braceClose+1);i=braceClose+1;continue;}
   const gap=text.slice(open+1,close).trim(); const children=text.slice(brace+1,braceClose);
   out+=text.slice(i,start)+`${name}(${gap}, {${children}})`; i=braceClose+1+suffix[0].length;
 }
 text=out;
 if(text!==original){fs.writeFileSync(file,text);changed.push(file);}
}
console.log(`Migrated ${changed.length} files.`); console.log(changed.join('\n'));
