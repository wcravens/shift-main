<!DOCTYPE html>
<!-- The error page when WC backend down -->
<html>
  <head>
    <title>Error:503 - SHIFT</title>
    <link rel="stylesheet" media="screen" href="https://fonts.googleapis.com/css?family=Source+Sans+Pro:300,400,600" />
    <link rel="stylesheet" media="screen" href="https://maxcdn.bootstrapcdn.com/font-awesome/4.3.0/css/font-awesome.min.css" />
    <style>
      * {
        -moz-box-sizing: border-box;
        -webkit-box-sizing: border-box;
        box-sizing: border-box;
      }
      .background.error-page-wrapper .content-container {
        border-radius: 6px;
        text-align: center;
        box-shadow: 0 0 50px rgba(0, 0, 0, .2);
        padding: 50px;
        background-color: #fff;
        max-width: 625px;

        width:100%;
        position:absolute;
        margin-left: auto;
        margin-right: auto;
      }
      .background.error-page-wrapper .head-line {
        transition: color .2s linear;
        font-size: 48px;
        line-height: 60px;
        letter-spacing: -1px;
        margin-bottom: 5px;
        color: #ccc;
      }
      .background.error-page-wrapper .subheader {
        transition: color .2s linear;
        font-size: 36px;
        line-height: 46px;
        color: #333;
      }
      .background.error-page-wrapper .hr {
        height: 1px;
        background-color: #ddd;
        width: 60%;
        max-width: 250px;
        margin: 35px auto;
      }
      .background.error-page-wrapper .context {
        transition: color .2s linear;
        font-size: 18px;
        line-height: 27px;
        color: #bbb;
      }
      .background.error-page-wrapper .buttons-container {
        margin-top: 35px;
        overflow: hidden;
      }
      .background.error-page-wrapper .buttons-container a {
        transition: text-indent .2s ease-out, color .2s linear, background-color .2s linear;
        text-indent: 0px;
        font-size: 14px;
        text-transform: uppercase;
        text-decoration: none;
        border-radius: 99px;
        padding: 12px 0 13px;
        text-align: center;
        display: inline-block;
        overflow: hidden;
        position: relative;
        width: 45%;
      }
      .background.error-page-wrapper .buttons-container a:hover {
      }
      .background-image {
        background-color: #98b4ce;
        background-image: url("") !important;
      }
      .primary-text-color {
        color: #494949 !important;
      }
      .secondary-text-color {
        color: rgba(152, 154, 158, 1) !important;
      }
      .button {
        background-color: rgb(152,180,206) !important;
        color: #FFFFFF !important;
      }
      .button:hover {
        background-color: rgba(45, 82, 204, 1) !important;
      }
      .center {
          width: 100px;
          height: 500px;
          background-color: red;

          position: absolute;
          top:0;
          bottom: 0;
          left: 0;
          right: 0;

          margin: auto;
      }
    </style>
  </head>

  <body class="background error-page-wrapper background-image" style="width:100%">
      <div class="content-container center"  >
        <div class="head-line secondary-text-color">
          503: SHIFT Backend
        </div>
        <div class="subheader primary-text-color">
          Looks like we're having some server issues.
        </div>
        <div class="hr"></div>
        <div class="context secondary-text-color">
          <p>
            Allow us 5 minutes to get back on our feet and try again. If this error persists, please report a problem.
          </p>
        </div>
        <div class="buttons-container single-button">
          <a id="refresh_button" class="button" href="\">To the main page</a>
        </div>
      </div>
  </body>
</html>