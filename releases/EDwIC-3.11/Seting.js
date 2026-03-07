// JavaScript Document
var json_config = new Object();
$(document).ready(function()
{
 update()
})

$('form').submit(function() {
  console.log($(this).serializeArray());
  var formArray = $('form').serializeArray();
  for (var i = 0; i < formArray.length; i++)
  {
    json_config[formArray[i]['name']] = formArray[i]['value'];
  }
  console.log(JSON.stringify(json_config));
  sendConfig();
  return false;
});

function sendConfig()
 {
  var str = $("form").serialize();
            $.post({
                    url: 'New_Seting',
                    data: JSON.stringify(json_config),
                    success: function()
                {
                  $('#save').css("background-color", "#43a047");
                  setTimeout(function eror() { $('#save').css("background-color", "#263238"); }, 500);
                } ,
                error: function()
                {
                  alert("ПОМИЛКА: сервер не відповідає");
                  $('#save').css("background-color", "#ff6e40");
                }
          });
 }

function update()
{
  $.ajax({
    type: "GET",
    url: "Config.json",
    dataType: "json",
    success:  function(data)
    {
      $("#SID_STA").val(data.SID_STA);
      $("#PAS_STA").val(data.PAS_STA);
      $("#TB_pasword").val(data.TB_pasword);
      $("#TB_Token").val(data.TB_Token);
    },
    error: function () { alert("ПОМИЛКА: сервер не відповідає"); }

  });
}
