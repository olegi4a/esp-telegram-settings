// JavaScript Document
let data_device = new Object();
$(document).ready(function()
{
 update();
})

function update()
{
  $.ajax({
    type: "GET",
    url: "/Update_data",
    dataType: "json",
    success:  function(data)
    {
      for (let key in data)
      {
        data_device[key] = data[key]
      }
      device();
      $('#menu_div').css("background-color", "#424242");
    },
    error: function () {
                         $('#menu_div').css("background-color", "#ff6e40");
                         $('#Rele').css("background-color", "#ff6e40");
                       }

  });
}

function device()
{
  if(data_device["Rele"] == 1)
  {
    $('#Rele').css("background-color", "#43a047");
  }
  else
  {
    $('#Rele').css("background-color", "#c62828");
  }
  if(data_device["TB_Status"] == 1)
  {
    $('#TB_Status').css("color", "#43a047");
  }
  else
  {
    $('#TB_Status').css("color", "#c62828");
  }
}

$('form').submit(function() {
  $.ajax({
    type: "GET",
    url: "Data",
    dataType: "text",
    success:  function(data){ update(); },
    error: function () { alert("ПОМИЛКА: сервер не відповідає"); }
  });
  return false;
});

setInterval(update, 20000);
