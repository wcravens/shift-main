<!-- send order form module -->
<!-- the action of this form is in sendorderform.js -->
<form id="sendorder_form" action="" method="post" class="form-inline" style="margin: 0px;">
    <input type="hidden" name="redirect_url" value="<?php echo $redirect_url;?>" />
    <input type="hidden" id="mysubmit" name="mysubmit" value=""/>
    <label class="sr-only" for="symbol"> Symbol: </label>

    <div class="form-group" style="padding-bottom: 10px; width: 80px">
        <select class="form-control" id="symbol" name="symbol">
            <?php  foreach ($stockList as $symbol) { ?>
                <option id="<?php echo $symbol; ?>_select" value="<?php echo $symbol; ?>"  >
                <?php echo $symbol; ?>
                </option>
            <?php }; ?>
        </select>
    </div>
    &nbsp &nbsp &nbsp 
    <div class="input-group" style="padding-bottom: 10px; width: 100px" >
        <span class="input-group-addon notselectable">$</span>
        <label class="sr-only" for="price_input"> Price: </label>
        <input class="form-control" id="price_input" type="number" step="0.01" min="0.01" name="price" placeholder="Price" style="width: 8em"/>
    </div>

    <div class="input-group" style="padding-bottom: 10px; width: 100px">
        <input class="form-control" id="quantity_input" type="number" min="1" name="size" placeholder="Size" style="width: 8em" onchange="quantityOnchange()"/>
        <label class="sr-only" for="quantity_input"> Size: </label>
        <span class="input-group-addon notselectable">x100 Shares</span>
    </div>    
    &nbsp &nbsp &nbsp 

    <div class="form-group">
        <div class="btn-group" style="padding-bottom: 10px; width: 200px">
            <input id="market_buy_btn" type="button" class="btn btn-default" style="width: 50%" value="Market Buy" />
            <input id="market_sell_btn" type="button" class="btn btn-default" style="width: 50%" value="Market Sell" />
        </div>
        &nbsp &nbsp &nbsp 
        <div class="btn-group" style="padding-bottom: 10px; width: 200px">
            <input id="limit_buy_btn" type="button" class="btn btn-default" style="width: 50%" value="Limit Buy " />
            <input id="limit_sell_btn" type="button" class="btn btn-default" style="width: 50%" value="Limit Sell " />
        </div>
    </div>

</form>


<?php
echo '<script src="/scripts/sendorderform.js?version='.$SHIFT_version.'"></script>';
?>