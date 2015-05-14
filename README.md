DO NOT USE!
===========

Since working on this project, I have come to the conclusion that using anything but xscreensaver is a very unwise decision ([details](http://www.jwz.org/xscreensaver/toolkits.html)). I am no longer updating this project and strongly advise against its use (or the use of the upstream branch). Since this program puts you in peril of disclosing sensitive information, I do think its general concept is a Bad Idea™. Imagine your friend bob writes an OTR-encrypted message to you via Jabber, which causes Pidgin to open a new window display the decrypted message.

If you just want to confuse/annoy people, I would suggest having xscreensaver take a screenshot when the screen is locked, which is then displayed by feh, xv or any other program capable of displaying images on X root windows. Another IMHO very nice (though seriously hacky) method is the one I described [in this gist](https://gist.github.com/jaseg/3487142): Have xscreensaver spawn an actual Windows XP VM that curious people can interact with, including automatic reset on screen unlocking.

Thanks for checking out anyway!

jaseg
