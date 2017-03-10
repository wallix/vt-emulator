let TTYHTMLRendering = (function () {

function i2strcolor (int_color) {
  return ('00000' + int_color.toString(16)).slice(-6)
}

function elem2style (e) {
  let style = 'color:#' + i2strcolor(e.f) + ';background-color:#' + i2strcolor(e.b) + ';'
  if (e.r) {
    if (e.r & 1) style += 'font-weight:bold;'
    if (e.r & 2) style += 'font-style:italic;'
    if (e.r & 4) style += 'text-decoration:underline;'
  }
  return style
}

function escaped (s) {
  return s.replace(/&/g, '&amp;').replace(/</g, '&lt;')
}

return function (screen) {
  const estyle = {
    r: screen.style.r||0,
    f: screen.style.f||0,
    b: screen.style.b||0
  }

  let emptyLine = '                                                               '
  while (emptyLine.length < screen.columns) {
    emptyLine += emptyLine
  }

  console.log(screen.x,screen.y)
  const htmlLineArray = screen.data.map(function (lines, y){
    return lines.reduce(function (htmlline, line){
        let lineLen = 0
        const isCursorY = (y === screen.y)
        htmlline = line.reduce(function (htmlline, e){
          if (e.r !== undefined) estyle.r = e.r
          if (e.f !== undefined) estyle.f = e.f
          if (e.b !== undefined) estyle.b = e.b

          const spanStyle = '<span style="' + elem2style(estyle) + '">'
          const s = e.s || ''
          if (isCursorY && lineLen <= screen.x && screen.x < s.length + lineLen) {
            const curstyle = { r: estyle.r, f: estyle.b, b: estyle.f }
            const spanStyle = '<span style="' + elem2style(estyle) + '">'
            const spanCurStyle = '<span style="' + elem2style(curstyle) + '">'
            const pcur = screen.x - lineLen
            htmlline += spanStyle + escaped(s.substr(0, pcur)) + '</span>'
              + spanCurStyle + escaped(s.substr(pcur, 1)) + '</span>'
              + spanStyle + escaped(s.substr(pcur + 1)) + '</span>'
          } else {
            htmlline += spanStyle + escaped(s) + '</span>'
          }
          lineLen += s.length
          return htmlline
        }, htmlline)
        if (isCursorY && lineLen <= screen.x) {
          htmlline += emptyLine.substr(0, screen.x - lineLen) + '<span style="'
            + elem2style({ r: estyle.r, f: estyle.b, b: estyle.f }) + '"> </span>'
        }
        return htmlline
    }, '')
  })

  return '<p id="tty-player-title">' + (screen.title||'') + '</p>'+
    '<div id="tty-player-terminal" style="color:#' +
      i2strcolor(screen.style.f) + ';background-color:#' +
      i2strcolor(screen.style.b) + '">' +
      '<p>' + htmlLineArray.join('\n</p><p>') + '\n</p>' +
      // force terminal width
      '<p style="height:1px">' + emptyLine.substr(0, screen.columns) + '</p>' +
    '</div>'
}

})()
